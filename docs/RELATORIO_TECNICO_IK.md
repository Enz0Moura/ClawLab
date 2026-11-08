# Relatório técnico — LaClaw Lab e cinemática inversa

## 1. Objetivo e escopo

O LaClaw Lab é um simulador 3D para o cenário de braço robótico do trabalho de Computação Gráfica. O jogador controla um braço articulado para capturar três aliens e entregá-los em uma bandeja. A aplicação foi escrita em C11, usa OpenGL/freeglut para janela, eventos e desenho, e implementa sua própria matemática vetorial, matrizes, câmera, árvore de cena e colisões AABB.

O controle original é cinemática direta (FK): o usuário define ângulos e o programa calcula a pose resultante. Foi adicionado um modo de cinemática inversa de posição (IK): o usuário indica um ponto na cena e o programa procura ângulos para aproximar o centro da garra desse ponto.

## 2. Funcionalidades do simulador

| Feature | Como funciona |
|---|---|
| Cena e câmera 3D | Projeção perspectiva; câmera orbital e livre; pan, zoom e picking por raio. |
| Braço articulado | Base, ombro, cotovelo e punho em hierarquia; palma e dedos dependem do punho. |
| Controle FK | Teclas 1–4 selecionam uma junta; setas, `+`/`-` e `[`/`]` alteram seus ângulos dentro dos limites. |
| Controle IK | `I` ativa a entrada de alvos por mouse. O algoritmo CCD calcula uma pose angular aproximada. |
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

O solver usa até 120 iterações, limita cada correção a 18° e considera alcançado um alvo quando o erro fica em até 0,05 unidade do cenário. Ele guarda a melhor pose encontrada, restaura os ângulos atuais e define os ângulos-alvo. O laço normal da simulação anima o braço até essa pose, preservando a resposta visual e a validação de colisões.

O erro mostrado na interface é a distância euclidiana atual entre o centro da garra e o alvo:

```text
erro = || posição_da_garra − posição_alvo ||
```

Com isso, o erro diminui durante a animação. O marcador fica verde dentro da tolerância e amarelo enquanto ainda há erro maior.

### 5.3 Interação

1. Pressione `I` para alternar de FK para entrada IK por mouse.
2. Clique em um alien disponível para usar o centro dele como alvo 3D.
3. Clique em espaço livre para definir um alvo no plano horizontal à altura do centro de um alien apoiado no piso (`y = 0,25`).
4. A garra se move para a melhor pose calculada. Pressione `C` para tentar capturar; a IK não fecha a garra automaticamente.
5. Pressione `I` novamente para voltar à seleção de aliens e ao controle FK manual.

O alvo é marcado na cena por três eixos curtos e um segmento que liga o efetuador ao alvo. O painel e a barra inferior mostram o modo e o erro atual.

## 6. IK, colisão e captura são etapas distintas

O solver otimiza a posição geométrica da garra e respeita os limites das juntas. Ele não planeja uma trajetória livre de obstáculos. Durante a animação, a validação preexistente compara AABBs do braço contra a cabine e os prêmios; se um passo atravessaria um obstáculo, o passo é desfeito e a interface indica bloqueio. Nesse caso, a garra pode parar antes do alvo e o erro continuará visível. O usuário pode escolher outro alvo ou refinar a pose manualmente.

Quando a garra chega perto do centro de um alien, a lógica de captura também verifica se o alien cabe entre os dedos e se há linha de visão sem obstáculo. A IK, portanto, aproxima o efetuador; a captura permanece uma regra independente do simulador.

## 7. Limitações e extensões possíveis

- O algoritmo resolve posição do centro da garra, não orientação do efetuador.
- O clique em espaço livre só escolhe pontos no plano horizontal de altura fixa. Cliques em aliens usam a posição 3D real do objeto.
- O CCD encontra uma solução geométrica aproximada. Para alvos inalcançáveis dentro do espaço de trabalho, aplica a melhor pose encontrada até o limite de iterações.
- O solver não inclui obstáculos na função de custo nem calcula caminho sem colisões; a validação interrompe movimentos inválidos depois.
- Não há autocolisão entre os segmentos do braço.
- As colisões são AABBs conservadoras, não volumes exatos das malhas.

Extensões didáticas naturais seriam incluir orientação-alvo, ponderar poses que preservem uma configuração preferida, testar a pose final contra obstáculos durante a busca, usar Jacobiano para comparação com CCD, ou calcular uma trajetória de aproximação e retirada antes de fechar a garra.

## 8. Roteiro para estudar e aplicar

1. Altere limites e ângulos iniciais em `src/app/config.c`; observe como a região alcançável muda.
2. Ajuste `MAX_ITERATIONS`, tolerância e correção máxima em `machine_solve_ik`; compare velocidade de convergência e erro final.
3. Teste alvos perto do braço, no limite de alcance e atrás de obstáculos. Diferencie alvo geometricamente inalcançável de alvo bloqueado por colisão.
4. Acompanhe os eixos das juntas com `D` e compare o eixo local com seu vetor transformado globalmente.
5. Compare os ângulos calculados pelo CCD com a manipulação manual FK para a mesma posição da garra.
6. Para apresentação, explique que FK avalia uma pose a partir dos ângulos, enquanto IK procura ângulos a partir de uma pose desejada; neste projeto, a busca é iterativa e sujeita a limites.

## 9. Arquivos principais

- [`machine.c`](../src/app/machine.c): cadeia de juntas, validação, captura e CCD.
- [`machine.h`](../src/app/machine.h): estado da máquina e interface pública do solver.
- [`main.c`](../src/app/main.c): tecla `I` e entrada de alvos por mouse.
- [`ui.c`](../src/app/ui.c): indicador espacial, modo e erro.
- [`scene.c`](../src/cg/scene.c): composição das transformações hierárquicas.
- [`config.c`](../src/app/config.c): medidas, limites e velocidade das juntas.
