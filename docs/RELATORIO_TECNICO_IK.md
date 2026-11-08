# Relatório técnico — LaClaw Lab e cinemática inversa

## 1. Objetivo e escopo

O LaClaw Lab é um simulador 3D para o cenário de braço robótico do trabalho de Computação Gráfica. O jogador controla um braço articulado para capturar três aliens e entregá-los em uma bandeja. A aplicação foi escrita em C11, usa OpenGL/freeglut para janela, eventos e desenho, e implementa sua própria matemática vetorial, matrizes, câmera, árvore de cena e colisões AABB.

O controle original é cinemática direta (FK): o usuário define ângulos e o programa calcula a pose resultante. O modo de cinemática inversa de posição (IK) projeta o cursor em um plano de altura fixa e procura ângulos para aproximar o centro da garra desse ponto. Clicar em um alien disponível inicia uma sequência automática de aproximação segura e captura.

## 2. Funcionalidades do simulador

| Feature | Como funciona |
|---|---|
| Cena e câmera 3D | Projeção perspectiva; câmera orbital e livre; pan, zoom e picking por raio. |
| Braço articulado | Base, ombro, cotovelo e punho em hierarquia; palma e dedos dependem do punho. |
| Controle FK | Teclas 1–4 selecionam uma junta; setas, `+`/`-` e `[`/`]` alteram seus ângulos dentro dos limites. |
| Controle IK | `I` ativa o seguimento do cursor. CCD calcula poses candidatas e um planejador de juntas busca uma rota livre de colisões. Clique num alien disponível para iniciar a captura automática. |
| Garra e carga | `C` fecha; `O` abre. A captura depende do alinhamento, da abertura e de não haver obstáculo no caminho. |
| Física dos aliens | Gravidade, colisões AABB, quiques, atrito e repouso simplificados. |
| Tarefa | Capturar os três aliens e deixá-los em repouso, diretamente dentro da bandeja. |
| Interface e depuração | Painel mostra ângulos, limites, estados; `D` mostra caixas de colisão, eixos e região útil. |

## 3. Organização e responsabilidades

- `src/cg/math.c`: vetores, matrizes 4×4, perspectiva, inversa de matriz, transformação de AABB e interseção raio-caixa.
- `src/cg/scene.c`: nós da cena, relações pai-filho, atualização das matrizes globais e desenho recursivo.
- `src/cg/camera.c`: câmera, projeção, pan/orbita e construção de raios a partir de coordenadas do mouse.
- `src/cg/geometry.c` e `src/app/theme.c`: primitivas e malhas com limites locais e normais.
- `src/app/machine.c`: hierarquia do braço, limites, validação de pose, seleção, captura e IK.
- `src/app/physics.c`: movimento e colisões dos prêmios soltos.
- `src/app/main.c` e `src/app/ui.c`: eventos de teclado/mouse, atualização temporal, painel, ajuda e desenho dos indicadores.

`Geometry` usa ponteiros de função para desenhar, mudar cor e destruir cada geometria. `SceneNode` guarda matriz local, matriz global, pai, filhos e geometria opcional. Isso separa a forma visual da posição hierárquica e permite que juntas sem geometria também participem da cadeia.

## 4. Transformações e cinemática direta

O projeto usa vetores coluna, matrizes armazenadas em ordem column-major e coordenadas com Y para cima. A matriz global de um filho é:

```text
W_filho = W_pai × L_filho
```

onde `L` é a transformação local e `W` é a transformação acumulada até a raiz. Para esta máquina, a cadeia principal é:

```text
Raiz da máquina
└── Base (rotação em Y)
    └── Ombro (rotação local em Z)
        └── Cotovelo (translação pelo elo superior + rotação em Z)
            └── Punho (translação pelo antebraço + rotação em Z)
                ├── Palma e centro da garra
                └── Pivôs dos dedos
```

A atualização em `machine.c:update_world` define as matrizes locais e chama a atualização recursiva da cena. O modelo mantém os elos em coordenadas locais; por isso, a rotação do ombro também move cotovelo, antebraço, punho e garra.

Os limites atuais são: base −110° a 65°, ombro −85° a 25°, cotovelo −135° a −15° e punho −110° a 80°. Eles são definidos em `src/app/config.c`. A abertura dos dedos varia de 0° a 40°.

## 5. IK por CCD

### 5.1 Conceito

Cyclic Coordinate Descent (CCD) ajusta uma junta por vez. Para cada junta, o algoritmo considera:

- `o`: origem da junta no espaço global;
- `a`: eixo global em torno do qual ela gira;
- `e`: posição atual do efetuador (centro da garra);
- `t`: posição-alvo.

Os vetores `e − o` e `t − o` são projetados no plano perpendicular a `a`. O ângulo assinado que alinha esses vetores é calculado por:

```text
delta = atan2(a · (u × v), u · v)
```

O algoritmo soma `delta` ao ângulo da junta e o limita ao intervalo permitido. Depois recalcula as matrizes globais, mede o erro e passa à junta anterior. Uma iteração completa percorre as juntas da extremidade até a base; várias iterações refinam a pose.

### 5.2 Aplicação neste braço

Em `machine_solve_ik`, o eixo da base é Y local e os outros três eixos são Z local. Cada eixo local é transformado pelo referencial global do pai antes de calcular o ângulo, para que o CCD trabalhe corretamente em coordenadas globais.

O solver usa até 120 iterações por configuração inicial, limita cada correção a 18° e considera alcançado um alvo quando o erro fica em até 0,05 unidade do cenário. Como os limites angulares podem formar ramos de solução diferentes, o CCD é reiniciado a partir da pose atual e de poses alternativas dentro dos limites. Poses finais que colidem com a cabine ou com prêmios são descartadas.

Para cada pose candidata válida, o planejador primeiro testa o segmento direto em espaço de juntas. Se houver uma colisão, usa uma árvore aleatória de exploração rápida (RRT) com até 1.200 nós para procurar uma sequência de configurações que conecte a pose atual ao destino. Cada segmento é amostrado em intervalos de até 0,5° por junta e validado pelo mesmo modelo AABB da simulação. A rota encontrada é simplificada por atalhos que também passam pela validação. A busca é determinística para o mesmo par de poses.

A rota fica armazenada no estado da máquina. Durante a animação, as juntas avançam de forma sincronizada ao longo de cada segmento validado; se um passo ficar inválido por uma mudança no cenário, o movimento é cancelado com segurança. Um novo clique substitui a rota em andamento. Um comando manual também cancela a rota e começa a partir da pose atual.

O erro mostrado na interface é a distância euclidiana atual entre o centro da garra e o alvo:

```text
erro = || posição_da_garra − posição_alvo ||
```

Com isso, o erro diminui durante a animação. O marcador fica verde dentro da tolerância e amarelo enquanto ainda há erro maior.

### 5.3 Interação

1. Pressione `I` para alternar de FK para o controle IK pelo cursor.
2. Mova o cursor sobre a cena. Cada posição é projetada no plano horizontal à altura do centro de um alien apoiado no piso (`y = 0,25`); a garra segue esse alvo quando termina a rota atual.
3. Clique em espaço livre para fixar um alvo nesse plano. Isso move a garra aberta e não inicia uma captura.
4. Clique em um alien disponível para iniciar a sequência de captura: a garra abre, planeja uma rota até o centro 3D do alien, espera chegar e verifica novamente a condição de captura antes de fechar.
5. Se o alien não estiver alinhado, se mover para fora da pose segura ou se não houver rota, a garra não fecha. Um alien já preso pode ser transportado ao mover o cursor.
6. Pressione `I` novamente para parar o controle pelo cursor e voltar ao controle FK manual.

O alvo é marcado na cena por três eixos curtos e um segmento que liga o efetuador ao alvo. O painel e a barra inferior mostram o modo e o erro atual.

## 6. IK, colisão e captura são etapas distintas

O solver otimiza a posição geométrica da garra, respeita os limites das juntas e planeja uma trajetória sem colisões dentro do orçamento de busca. O planejador usa AABBs, portanto a validação é conservadora e pode rejeitar passagens estreitas. Se nenhuma rota for encontrada dentro do orçamento, o braço permanece onde está e o painel informa que não encontrou rota. Para um alvo geometricamente inalcançável, o solver ainda pode agendar a melhor aproximação segura encontrada; o retorno indica que a tolerância de 0,05 não foi atingida.

O clique em alien inicia uma máquina de estados separada do solver IK: abertura, aproximação, validação e fechamento. Antes de fechar, a interface exige que `machine_capture_candidate` identifique exatamente o alien clicado. Durante o fechamento, essa condição é verificada antes de cada passo; se o alien sair da pose segura, o fechamento é interrompido e a garra reabre. O detector também verifica se o alien cabe entre os dedos e se há linha de visão sem obstáculo. Assim, o braço não empurra o alien para tentar pegá-lo.

## 7. Limitações e extensões possíveis

- O algoritmo resolve posição do centro da garra, não orientação do efetuador. A sequência automática usa a checagem independente de captura antes de fechar.
- O seguimento e os cliques em espaço livre usam o plano horizontal de altura fixa. O clique em alien usa a posição 3D real e acompanha pequenos deslocamentos do alien durante a aproximação.
- O CCD encontra soluções geométricas aproximadas; o planejamento de rota é limitado a 1.200 nós e pode falhar mesmo quando existe um caminho mais complexo.
- O RRT avalia colisões por AABB amostradas em incrementos de até 0,5°; peças estreitas ou a geometria real das malhas podem diferir das caixas de colisão.
- Não há autocolisão entre os segmentos do braço.
- As colisões são AABBs conservadoras, não volumes exatos das malhas.

Extensões didáticas naturais seriam incluir orientação-alvo, ponderar poses que preservem uma configuração preferida, comparar CCD com Jacobiano, testar malhas com volumes de colisão mais precisos ou planejar uma trajetória de aproximação e retirada antes de fechar a garra.

## 8. Roteiro para estudar e aplicar

1. Altere limites e ângulos iniciais em `src/app/config.c`; observe como a região alcançável muda.
2. Ajuste o limite de iterações do CCD, a tolerância e a correção máxima em `solve_ik_seed`; compare velocidade de convergência e erro final.
3. Ajuste `IK_RRT_MAX_NODES` e `IK_RRT_MAX_ANGLE_STEP`; compare tempo de busca, suavidade e taxa de rotas válidas.
4. Teste alvos perto do braço, no limite de alcance e atrás de obstáculos. Diferencie alvo geometricamente inalcançável de alvo sem rota segura encontrada no orçamento.
5. Acompanhe os eixos das juntas com `D` e compare o eixo local com seu vetor transformado globalmente.
6. Compare os ângulos calculados pelo CCD com a manipulação manual FK para a mesma posição da garra.
7. Para apresentação, explique que FK avalia uma pose a partir dos ângulos, enquanto IK procura ângulos a partir de uma pose desejada; neste projeto, a busca é iterativa, sujeita a limites e seguida de planejamento em espaço de juntas.

## 9. Arquivos principais

- [`machine.c`](../src/app/machine.c): cadeia de juntas, validação, captura e CCD.
- [`machine.h`](../src/app/machine.h): estado da máquina e interface pública do solver.
- [`main.c`](../src/app/main.c): tecla `I` e entrada de alvos por mouse.
- [`ui.c`](../src/app/ui.c): indicador espacial, modo e erro.
- [`scene.c`](../src/cg/scene.c): composição das transformações hierárquicas.
- [`config.c`](../src/app/config.c): medidas, limites e velocidade das juntas.
