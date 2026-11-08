# Manual do usuário — LaClaw Lab

**Português (Brasil)** | [English](USER_MANUAL.md)

[Voltar ao README](../README.md)

## 1. Objetivo

Você controla um braço articulado, pega três aliens — Coral, Azul e Amarelo — e
solta cada um na bandeja verde. Um alien só conta quando está solto, parado
diretamente sobre a bandeja e inteiro dentro dela. Com **3/3**, a coleção está
completa.

Você define o ângulo de cada junta (cinemática direta). Clicar num alien só o
destaca; o braço não vai até ele sozinho. A captura segue uma regra geométrica
fixa: não há sorte envolvida, e o alien não escapa enquanto está preso.

### Primeira partida

Use o Sandbox, que não tem limite de tempo:

1. Abra o simulador e pressione **1**.
2. Clique num alien para destacá-lo (opcional; serve só para saber qual você quer pegar).
3. Escolha uma junta com **1–4** e mova com **←/→**. Com a garra aberta (40°) e o
   braço elevado, gire a base até o alien, ajuste cotovelo e punho e só então
   abaixe o ombro.
4. Quando o painel mostrar `Captura possivel`, feche a garra com **C**. O painel
   passa a mostrar `Garra ocupada`.
5. Levante o ombro e gire a base até a bandeja (algo entre −75° e −105°).
6. Espere o braço parar e abra a garra com **O**. Quando o estado do alien mudar
   para `Entregue`, o contador sobe.

Repita com os outros dois. **H** abre a ajuda dentro do jogo.

### Modos

O menu aparece ao abrir o simulador. Clique numa opção ou pressione **1–4**:

| Tecla | Modo | Tempo | Bandeja (largura × profundidade) | Comportamento |
| --- | --- | --- | --- | --- |
| `1` | Sandbox | Sem limite | 2,50 × 1,60 | Pouca variação de posição e quiques suaves |
| `2` | Fácil | 4 minutos | 2,35 × 1,40 | Pouca variação de posição e quiques leves |
| `3` | Médio | 3 minutos | 2,20 × 1,20 | Ângulos e distâncias mais variados; quiques moderados |
| `4` | Difícil | 2 minutos | 2,05 × 1,00 | Maior variação de posição; quiques fortes e demorados |

No topo da tela ficam o modo e o tempo restante. Abaixo de 30 segundos, o
cronômetro fica vermelho.

### Bandeja de entrega

A bandeja diminui a cada dificuldade, então a soltura exige mais precisão. As
medidas da tabela estão em unidades do cenário; o centro e a altura não mudam.
No Difícil, a área é cerca de 49% menor que no Sandbox, mas ainda cabem três
aliens lado a lado, sem empilhar. As divisórias desenhadas na bandeja e o rótulo
`ENTREGA` acompanham o tamanho.

Alinhe cada alien com um espaço livre, espere o braço parar e só então abra a
garra: soltar em movimento passa velocidade para o alien, e ele pode passar da
borda. Todo layout sorteado é validado com uma folga de pelo menos 0,05 unidade
entre o alien e as bordas da bandeja.

O reset mantém o tamanho da bandeja do modo atual. Pausar e continuar preserva a
bandeja e as entregas.

### Posições dos aliens

As posições são sorteadas a cada nova partida e a cada reset, e as cores também
trocam de lugar (o alien azul nem sempre fica no centro). Todos começam em pé,
apoiados no piso, sem sobreposição e longe da bandeja e dos obstáculos.

- **Sandbox e Fácil:** distância constante até a base e pouca variação de ângulo.
- **Médio:** três distâncias possíveis.
- **Difícil:** sete distâncias e uma faixa de ângulo maior; você precisa ajustar
  ombro, cotovelo e punho em cada alien.

Cada layout passa por uma verificação que simula aproximação, captura, retirada e
transporte até a bandeja, então todo alien é alcançável. Depois de capturar,
levante o braço antes de girar. Use `[` e `]` para o ajuste fino de 1°.

### Pausa e fim da rodada

**P**, **Esc** ou o botão **P PAUSA** abre a pausa. O cronômetro, o braço e os
quiques ficam congelados. A ajuda também congela o tempo. A pausa tem três opções:

- **Continuar** (`P` ou `Esc`): retoma exatamente de onde parou — tempo, posições,
  velocidades, seleção, câmera, abertura da garra, alien preso e pontuação. Não
  cria outra partida.
- **Resetar partida** (`R`): reinicia o modo atual, com cronômetro e câmera, e
  sorteia novas posições.
- **Voltar aos modos** (`M`): encerra a partida e abre a tela de seleção.
  Escolher um modo começa uma partida nova.

Com a pausa aberta, o botão do topo vira **P CONTINUAR**, e os controles do jogo
ficam bloqueados. Ao continuar, comece um novo gesto para voltar a mover com as
setas ou arrastar a câmera. Pausar, continuar e abrir ou fechar a ajuda não
refazem o sorteio. **R** também reinicia durante o jogo.

A rodada termina quando você entrega os três aliens ou o tempo acaba, e a tela
mostra o resultado. Use **R** para tentar de novo, ou abra a pausa para voltar aos
modos. Continuar depois do resultado não reabre a rodada. Para sair, feche a
janela ou pressione **Esc** na tela de modos.

## 2. Preparação e execução

Ambiente em que o projeto foi testado: **CachyOS Linux (base Arch)**, sessão
X11/XWayland, GCC 16.2.1, Clang 23.1.3, Make 4.4.1, freeglut 3.8.0, libglvnd
1.7.0 e NVIDIA RTX 2060 com OpenGL 4.6 de compatibilidade (driver 615.78.08).
Windows e macOS não foram testados. O programa usa o pipeline de compatibilidade
e não exige OpenGL 4.6.

No CachyOS/Arch, instale as dependências com:

```sh
sudo pacman -S --needed base-devel freeglut libglvnd
```

Abra um terminal **na pasta do projeto**, a que contém o `Makefile`, e execute:

```sh
make
./build/prize_machine
```

Ou use só `make run`. Não rode o programa com `sudo`. A janela abre em 1280 × 800;
recomendo pelo menos 1100 × 720, porque abaixo disso textos e controles ficam
apertados. A câmera se adapta à proporção da área 3D, e o painel se compacta na
vertical.

Para ver só a biblioteca:

```sh
make lib
make example
```

O exemplo mostra um cubo alongado e um cilindro que acompanha a rotação do pai.
Nele, as setas controlam a câmera, `+`/`-` o zoom e `Esc` fecha. Os controles do
simulador estão na próxima seção.

## 3. Todos os controles do simulador

| Ação | Comando |
| --- | --- |
| Selecionar um prêmio disponível | Clique esquerdo diretamente no alien |
| Selecionar base / ombro / cotovelo / punho | `1` / `2` / `3` / `4` |
| Selecionar junta pelo painel | Clique na linha da junta |
| Diminuir / aumentar o ângulo continuamente | Segure `←` / `→` |
| Diminuir / aumentar o alvo em 5° | Toque `-` / `+` (ou `=` para aumentar) |
| Ajustar o alvo em −1° / +1° | `[` / `]` |
| Ajustar 5° pelo mouse | Botões `−` / `+` na linha da junta |
| Fechar a garra | `C` ou botão **C FECHAR** |
| Abrir e soltar o prêmio | `O` ou botão **O ABRIR** |
| Alternar fechar / abrir | `Espaço` |
| Girar a câmera | Arraste com o botão direito na cena |
| Deslocar a câmera na horizontal e na vertical | Arraste com a roda pressionada (botão do meio) |
| Alternar câmera orbital / livre | `F` |
| Aproximar / afastar a câmera | Roda do mouse na cena |
| Restaurar a câmera | `Home` |
| Ajuda | `H`, `F1` ou botão **H AJUDA** |
| Visualização de depuração | `D` |
| Reiniciar a cena e a câmera | `R` ou botão **R REINICIAR** |
| Pausar / continuar | `P`, `Esc` ou botão **P PAUSA** / **P CONTINUAR** |
| Reiniciar pela pausa | **R RESETAR PARTIDA** ou `R` |
| Voltar à tela de modos | **M VOLTAR AOS MODOS** ou `M`, dentro da pausa |
| Sair | Fechar a janela ou `Esc` na tela de modos |

As letras funcionam em maiúsculas e minúsculas. Nos incrementos de 5° e 1°, solte
a tecla a cada passo: segurar `+` não repete. Para movimento contínuo, use as
setas; soltá-las interrompe o movimento. Câmera e seleção usam botões do mouse
diferentes para não se misturarem, e, com a ajuda aberta, não dá para selecionar
objetos atrás do painel.

No modo livre, o botão direito olha ao redor a partir da posição atual, e a roda
avança ou recua na direção da visão. A roda pressionada desloca a câmera nos dois
modos. Comece os gestos dentro da cena: o painel e a ajuda não iniciam movimento
de câmera. Soltar o botão encerra o gesto. `Home` e `R` voltam à posição inicial
e ao modo orbital. A câmera livre atravessa a cabine e pode olhar abaixo do
horizonte.

## 4. O que aparece na tela

**Cena:** a máquina segue o tema Pizza Planet, de Toy Story: foguete vermelho,
topo amarelo, aletas laterais, faixa azul e estrelas. O braço é metálico, com
articulações vermelhas. Os prêmios são aliens verdes de três olhos e antena, com
uniformes **Coral, Azul e Amarelo**, que correspondem aos rótulos na tela. A
plataforma verde com três espaços é a bandeja de entrega. A abertura escura na
parte externa do foguete é só decoração. A frente é aberta para você ver o braço.

**Seleção:** um contorno verde-limão acompanha o alien selecionado, e a linha dele
fica destacada na lista. Clicar num espaço vazio, ou numa superfície que tapa o
alien, limpa a seleção. Selecionar não aumenta o alcance da garra.

**Tripulação:** mostra a contagem e o estado de cada alien: `Disponivel`,
`Na garra`, `Em movimento` ou `Entregue`. Um alien entregue continua visível na
bandeja, mas não pode ser capturado de novo. O rótulo dele some da cena para não
cobrir os outros.

**Juntas:** a linha verde-limão é a junta ativa. O número grande é o ângulo
**atual**, em graus, e o intervalo entre colchetes é o limite. Os ângulos mudam
aos poucos depois de você pressionar os botões.

| Junta | Inicial | Limites |
| --- | ---: | ---: |
| Base | 0° | −110° a 65° |
| Ombro | −25° | −85° a 25° |
| Cotovelo | −85° | −135° a −15° |
| Punho | −70° | −110° a 80° |
| Dedos | 40°, abertos | 0° a 40° |

Uma colisão pode barrar o movimento antes do limite angular. O intervalo é a
capacidade da junta, não garante que qualquer combinação de ângulos caiba na
cabine.

**Garra:** a barra mostra a abertura. Ao capturar, os dedos param perto de 20°,
sem esmagar o alien. O texto abaixo da barra indica a situação:
`Aproxime a garra aberta`, `Captura possivel` (alinhamento suficiente),
`Garra ocupada` (o alien acompanha o braço) e `Colecao completa!`.

**Rodapé:** avisa captura, soltura, limite de junta, bloqueio e entrega.
`Movimento bloqueado` quer dizer que a última tentativa de avanço foi rejeitada;
mova a junta no sentido contrário ou ajuste outra para sair da posição.

**Depuração (D):** mostra as caixas de colisão, os eixos coloridos, a posição da
garra e o número de movimentos rejeitados. A caixa rosa é a tolerância para o
**centro** do alien, o volume verde é a região de entrega e a caixa externa
marca os limites da cabine. Serve para estudar a simulação; não é necessária para
jogar.

## 5. Regras do jogo

- A captura exige garra vazia, alien disponível perto do centro da garra, tamanho
  compatível e caminho livre. Fechar longe não captura nada.
- Os dedos precisam estar abertos para receber o alien. A seleção é opcional na
  captura: vale a proximidade real, não o nome destacado.
- Depois de capturado, o alien acompanha posição e orientação da garra, sem salto
  de alinhamento. O fechamento para quando o espaço é suficiente.
- O braço e a carga não atravessam piso, fundo, colunas, batente, bandeja, outros
  aliens nem os limites internos da cabine. Os lados abertos deixam você olhar,
  mas continuam sendo limites da área de jogo.
- Algumas caixas de colisão são maiores que a forma visível quando as peças estão
  inclinadas, então o bloqueio pode acontecer um pouco antes do contato aparente.
- Abrir a garra solta o alien onde ele está, com a velocidade do braço. Ele mantém
  a orientação, cai com gravidade e quica no piso, na cabine, no braço e em outros
  aliens. A dificuldade define a força do quique e o amortecimento. Espere o estado
  `Disponivel` antes de tentar recapturar um alien em movimento.
- Para contar, o alien precisa parar diretamente sobre a bandeja e caber inteiro
  nela. Passar por cima ainda preso não conta, e pousar na borda ou em cima de
  outro alien também não.
- A entrega só se desfaz num reinício, e cada alien conta uma vez.
- `R` sorteia novas posições e restaura aliens, ângulos, seleção, contagem, câmera,
  junta ativa e o tempo do modo atual. Ajuda e depuração mantêm o estado em que
  estavam.

## 6. Problemas comuns

**“GL/freeglut.h: No such file” ou “cannot find -lglut”:** faltam os headers ou a
biblioteca freeglut. Instale os pacotes da seção 2 e compile de novo.

**“failed to open display”:** rode numa sessão gráfica local com acesso ao
display. Um terminal remoto sem encaminhamento gráfico não abre a janela.
`make test` e `make sanitize` funcionam sem janela.

**A janela abre, mas o braço não responde:** clique na janela para dar foco,
selecione uma junta com `1`–`4` e use as setas. Nos incrementos, solte e pressione
a tecla de novo. Veja se o rodapé indica bloqueio ou limite.

**Não consigo selecionar o alien:** clique no alien, não no texto que flutua sobre
ele. Feche a ajuda. Uma coluna, uma peça do braço ou outro alien pode estar na
frente; gire a câmera com o botão direito. `Home` restaura a vista inicial.

**A garra fecha sem pegar:** abra com `O` e espere chegar a 40°. Ajuste as juntas
até aparecer `Captura possivel`. Se a garra já estiver muito fechada, o alien não
cabe.

**O alien ficou difícil de pegar depois de cair:** a orientação é preservada, e
algumas posições inclinadas não cabem na garra. Reposicione a garra ou use `R`;
o reinício sempre devolve os três aliens a posições alcançáveis.

**O alien está na bandeja, mas não pontuou:** ele precisa estar solto, apoiado
diretamente na bandeja, parado e totalmente dentro dela. Espere os quiques
acabarem. Uma pilha ou a borda não conta como entrega.

**Textos pequenos ou sobrepostos:** aumente a janela para 1280 × 800. O texto da
interface não usa acentos, por compatibilidade com a fonte bitmap do freeglut;
isso não indica problema na instalação.

## 7. Limitações e testes

É uma simulação didática de cinemática direta. Não inclui IK, física realista dos
dedos, autocolisão entre os segmentos do braço, deformação de pelúcia, som,
persistência entre execuções nem salvamento de partida. A câmera pode mostrar o
verso das peças, e não há vidro transparente nem sombras projetadas. As colisões
usam caixas conservadoras e não giram os aliens.

Rode os testes de lógica e memória com:

```sh
make test       # leva cerca de um minuto
make sanitize   # os mesmos testes com Clang, ASan e UBSan
```

Eles cobrem a entrega dos três aliens em todos os modos, quiques, colisões entre
aliens, tempo esgotado, reinício e falhas de alocação. Também percorrem 24 seeds
por modo, com as seis ordens de coleta, apoio no piso, ausência de sobreposição e
variação crescente com a dificuldade. Para a bandeja, verificam que ela diminui a
cada modo, mantém o tamanho no reset, deixa folga para as três entregas e não
pontua um alien parcialmente na borda.

O gerador de posições usa poses de cinemática direta, um número limitado de
tentativas e um layout seguro de reserva, que também é validado.
`machine_create_seeded(mode, seed)` reproduz um layout nos testes; numa partida
normal, a seed é escolhida automaticamente.

Para rodar a sessão gráfica automática, que precisa de display:

```sh
make smoke
```

Ela abre e fecha o exemplo e depois o simulador, usa os controles, compara o
estado antes e depois da pausa (inclusive com um alien preso e com aliens em
movimento), testa o retorno aos modos e o reset, e salva capturas de tela em
`build/smoke_*.ppm`. É só uma ferramenta de teste; o jogo normal, iniciado com
`make run`, é inteiramente manual.
