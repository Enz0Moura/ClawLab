# Manual do usuário — LaClaw Lab

**Português (Brasil)** | [English](USER_MANUAL.md)

[Voltar ao README](../README.md)

## 1. Objetivo

Pegue os três prêmios — Coral, Azul e Amarelo — com o braço articulado e solte-os
na bandeja verde. Cada prêmio conta uma vez, depois de parar diretamente na bandeja solto.
Ao chegar a **3/3**, a coleção está completa. O sandbox não tem tempo limite;
os desafios têm cronômetro. A captura não depende de sorte e o prêmio não escapa enquanto preso.

O jogador controla os ângulos das juntas. Clicar num prêmio apenas o destaca;
o braço não se move automaticamente até ele.

### Escolher um modo

O menu aparece ao abrir o simulador. Clique numa opção ou pressione **1–4**:

| Tecla | Modo | Tempo | Comportamento |
| --- | --- | --- | --- |
| `1` | Sandbox | Sem limite | Pequena variação de posição e quiques suaves |
| `2` | Fácil | 4 minutos | Pequena variação de posição e quiques leves |
| `3` | Médio | 3 minutos | Ângulos e distâncias mais variados; quiques moderados |
| `4` | Difícil | 2 minutos | Maior variação de posição; quiques fortes e demorados |

As posições dos aliens são sorteadas a cada nova partida e reset. As cores também
trocam de lugar: o alien azul, por exemplo, não fica sempre no centro. No sandbox
e no fácil, a distância até a base é constante e o ângulo varia pouco. No médio,
há três distâncias possíveis; no difícil, sete distâncias e uma faixa angular
maior exigem ajustar também ombro, cotovelo e punho.

Todos começam em pé, apoiados no piso, sem sobreposição, longe da bandeja e dos
obstáculos. Cada layout passa por uma verificação da rota de aproximação, captura,
retirada e transporte. Para se aproximar, gire a base com a garra elevada, ajuste
cotovelo e punho e então abaixe o ombro. Depois de capturar, eleve o braço antes
de girar rumo à bandeja. Use `[` e `]` para o alinhamento fino de 1°.

Durante a partida, **1–4** voltam a selecionar as juntas. O topo mostra o modo
e o tempo restante; abaixo de 30 segundos, o cronômetro fica vermelho.
**P**, **Esc** ou o botão **P PAUSA** abre a pausa. O cronômetro, os movimentos do
braço e os quiques dos aliens ficam congelados. A pausa oferece três opções:

- **Continuar** (`P` ou `Esc`): retoma o mesmo tempo restante, posições, velocidades,
  seleção, câmera, abertura da garra, prêmio capturado e pontuação. Não cria outra partida.
- **Resetar partida** (`R`): reinicia o modo atual, incluindo cronômetro e câmera,
  e sorteia novas posições para os aliens.
- **Voltar aos modos** (`M`, dentro da pausa): encerra a partida atual e abre a tela
  de seleção. Escolher um modo começa uma nova partida.

O botão do topo muda para **P CONTINUAR** enquanto a pausa está aberta. Os controles
do jogo atrás dela ficam bloqueados; para mover de novo com setas ou arrastar a
câmera, comece um novo gesto depois de continuar. A ajuda também congela o tempo.
Pausar, continuar e abrir ou fechar a ajuda não refazem o sorteio das posições.
**R** continua disponível para reiniciar durante o jogo.
Ao entregar os três aliens ou esgotar o tempo, a rodada para e mostra o resultado.
Use **R** para tentar novamente ou abra a pausa para voltar aos modos. Continuar
após o resultado mantém a rodada encerrada. Para sair, feche a janela ou pressione
**Esc** na tela de modos.

## 2. Preparação e execução

O ambiente efetivamente verificado foi **CachyOS Linux (base Arch)**, em sessão
com display X11/XWayland `:0`, GCC 16.2.1, Clang 23.1.3, Make 4.4.1,
freeglut 3.8.0, libglvnd 1.7.0 e NVIDIA RTX 2060 com OpenGL 4.6 de
compatibilidade, driver 615.78.08. Não foi verificado suporte a Windows ou macOS.
O programa usa recursos do pipeline de compatibilidade, não exige OpenGL 4.6.

No CachyOS/Arch, os pacotes necessários podem ser instalados com:

```sh
sudo pacman -S --needed base-devel freeglut libglvnd
```

Abra um terminal **na pasta do projeto** e execute:

```sh
make
./build/prize_machine
```

Ou use apenas `make run`. Não execute o programa com `sudo`.
A janela abre em 1280 × 800. Recomenda-se pelo menos 1100 × 720; abaixo disso,
textos e controles podem ficar próximos demais. A câmera se adapta à proporção
da área 3D, e o painel se compacta verticalmente.

Para conhecer apenas a biblioteca:

```sh
make lib
make example
```

O exemplo mostra um cubo alongado e um cilindro que acompanha a rotação de seu
pai. Nesse exemplo, as setas controlam a câmera, `+`/`-` controlam o zoom e
`Esc` encerra. Os comandos do simulador são os da próxima seção.

## 3. Todos os controles do simulador

| Ação | Comando |
| --- | --- |
| Selecionar um prêmio disponível | Clique esquerdo diretamente no alienígena |
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
| Deslocar a câmera lateralmente e na vertical | Arraste com a roda pressionada (botão do meio) |
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

As letras funcionam em maiúsculas e minúsculas. Para os incrementos de 5° e 1°,
pressione e solte a tecla a cada passo; segurar `+` não repete os passos.
Use as setas para movimento contínuo. Soltá-las interrompe esse movimento.
Os botões da câmera e da seleção são diferentes para não misturar as ações.
A ajuda aberta impede selecionar objetos atrás do painel de ajuda.

No modo livre, arraste com o botão direito para olhar ao redor a partir da
posição atual; a roda avança ou recua na direção da visão. A roda pressionada
desloca a câmera nos dois modos. Inicie os gestos dentro da cena; o painel e
a ajuda não iniciam movimentos da câmera. Soltar o botão encerra o gesto.
`Home` e `R` restauram a posição inicial e o modo orbital. A câmera livre
pode atravessar a cabine e olhar abaixo da linha do horizonte.

## 4. O que aparece na tela

**Cena:** a máquina tem o tema Pizza Planet, inspirado em Toy Story:
foguete vermelho, topo amarelo, aletas laterais, faixa azul e estrelas. O braço
metálico tem articulações vermelhas. Os três prêmios são alienígenas verdes de
três olhos, antena e uniformes nas cores **Coral, Azul e Amarelo**, correspondentes
aos rótulos na tela; a pele dos três é verde. A plataforma verde
com três espaços continua sendo a bandeja de entrega. A abertura escura na
parte externa do foguete é decorativa. A frente aberta permite ver o braço.

**Seleção:** um contorno verde-limão acompanha o alienígena selecionado e sua linha fica
destacada na coleção. Clicar num espaço vazio ou numa superfície que bloqueia
um prêmio limpa a seleção. O destaque não dá alcance extra à garra.

**Tripulação:** mostra a contagem e os estados `Disponivel`, `Na garra`, `Em movimento`
ou `Entregue`. Um prêmio entregue permanece visível na bandeja, mas não pode ser
capturado novamente. Seus rótulos na cena desaparecem para não encobrir os demais.

**Juntas:** a linha verde-limão é a junta ativa. O número grande é o ângulo
**atual**, em graus; o intervalo entre colchetes é o limite. Os ângulos mudam
gradualmente depois de pressionar os botões. Aguarde o valor desejado antes de
executar a próxima etapa do tutorial.

| Junta | Inicial | Limites |
| --- | ---: | ---: |
| Base | 0° | −110° a 65° |
| Ombro | −25° | −85° a 25° |
| Cotovelo | −85° | −135° a −15° |
| Punho | −70° | −110° a 80° |
| Dedos | 40°, abertos | 0° a 40° |

Uma colisão pode impedir o avanço antes do limite angular. O intervalo indica a
capacidade da junta, não uma garantia de que toda combinação das juntas cabe na cabine.

**Garra:** a barra indica abertura. Durante a captura, os dedos param em torno
de 20°, sem esmagar o alienígena. `Captura possivel` indica alinhamento suficiente;
`Garra ocupada` confirma que o prêmio acompanha o braço.

**Rodapé:** informa captura, soltura, bloqueio e entrega. `Movimento bloqueado`
significa que a última pequena tentativa de avanço foi rejeitada. Mude a junta
no sentido contrário ou ajuste outra junta para sair da posição.

**Depuração (D):** mostra caixas, eixos coloridos, posição da garra e o número
de movimentos rejeitados. A caixa rosa marca a tolerância para o **centro** do
prêmio. O volume verde marca a entrega. A caixa externa marca os limites da
cabine. Esse modo ajuda a estudar e não é necessário para jogar.

## 5. Regras do jogo

- A captura exige garra vazia, prêmio disponível próximo do centro da garra,
  tamanho compatível e caminho desobstruído. Fechar longe não captura nada.
- Os dedos precisam estar abertos para receber o alienígena. A seleção é opcional
  para a captura: vale a proximidade real, não o nome destacado.
- Depois de capturado, o prêmio acompanha posição e orientação da garra sem
  um salto de alinhamento. O fechamento para quando o espaço é suficiente.
- O braço e a carga não podem atravessar piso, fundo, colunas, batente,
  bandeja, demais prêmios ou os limites internos da cabine. Os lados abertos
  permitem olhar, mas continuam sendo limites da área de jogo.
- Algumas caixas são maiores que a forma visível quando as peças estão
  inclinadas. Pode haver bloqueio um pouco antes do contato aparente.
- Abrir solta o prêmio na posição atual, com a velocidade do braço. Ele mantém
  a orientação, cai sob gravidade e quica ao colidir com piso, cabine, braço
  e outros aliens. A dificuldade controla a força do quique e o amortecimento.
  Espere o estado `Disponivel` para recapturar um alien em movimento.
- Para contar, o prêmio deve parar diretamente sobre a bandeja e caber
  inteiramente nela. Passar por cima ainda preso não conta. Pousar parcialmente
  na borda ou em outro prêmio também não conta.
- A entrega é irreversível até reiniciar. Cada prêmio incrementa uma vez.
- `R` sorteia novos spawns e restaura prêmios, ângulos, seleção, contagem, câmera,
  junta ativa e tempo do modo atual.
  As opções de mostrar ajuda e depuração conservam sua preferência atual.

## 6. Problemas comuns

**“GL/freeglut.h: No such file” ou “cannot find -lglut”:** faltam os headers ou
a biblioteca freeglut. Instale os pacotes da seção 2 e compile novamente.

**“failed to open display”:** execute em uma sessão gráfica local com acesso
ao display. Um terminal remoto sem encaminhamento gráfico não abre a janela.
`make test` e `make sanitize` continuam funcionando sem janela.

**A janela aparece, mas o braço não responde:** clique para dar foco à janela,
selecione uma junta com `1`–`4` e use as setas. Para incrementos, solte e
pressione de novo. Confira se o rodapé informa um bloqueio ou limite.

**Não consigo selecionar o prêmio:** clique no alienígena, não no texto suspenso.
Feche a ajuda. Uma coluna, peça do braço ou outro prêmio pode estar na frente;
gire a câmera com o botão direito. `Home` restaura a vista inicial.

**A garra fecha sem pegar:** abra com `O` e aguarde 40°. Ajuste as juntas até
`Captura possivel`. Se já estiver muito fechada, ela não consegue receber o alienígena.

**O prêmio ficou difícil de pegar após cair:** a orientação é preservada e
algumas posições inclinadas não cabem na garra. Reposicione-a ou use `R`.
Reiniciar sempre retorna aos três prêmios em posições utilizáveis.

**Prêmio na bandeja, mas sem ponto:** ele deve estar solto, apoiado diretamente
na bandeja, parado e totalmente dentro dela. Espere os quiques terminarem.
Uma pilha ou a borda não é uma entrega.

**Textos pequenos ou sobrepostos:** amplie a janela para 1280 × 800.
Os textos da interface usam caracteres sem acentos por compatibilidade com a
fonte bitmap do freeglut; isso não indica problema na instalação.

## 7. Limitações e verificações

Esta é uma simulação didática de cinemática direta. Não inclui IK, física
realista dos dedos, autocolisão entre segmentos, deformação de pelúcia, som,
persistência entre execuções ou salvamento de partida. A câmera pode mostrar
o verso das peças; não há vidro transparente nem sombras projetadas.
As colisões usam caixas conservadoras e não provocam rotação dos aliens.

Os testes verificam a entrega dos três aliens em todos os modos, quiques,
colisões entre prêmios, tempo esgotado, reinício e falhas de alocação.
Também verificam 24 sementes por modo, cobrindo as seis ordens de coleta,
o apoio no piso, a ausência de sobreposição e o aumento da variação por dificuldade.
O gerador usa poses de cinemática direta, tentativas limitadas e um layout seguro
de reserva, também validado. `machine_create_seeded(mode, seed)` permite reproduzir
um layout nos testes; a partida normal escolhe uma nova semente automaticamente.
O teste gráfico compara o estado antes e depois da pausa, inclusive durante
uma captura e com aliens em movimento, e verifica o retorno aos modos e o reset.
Execute os testes de lógica e memória com `make test` e `make sanitize`.

Para reproduzir a sessão automática de verificação:

```sh
make smoke
```

Ela abre e fecha o exemplo e depois a máquina, exercita os controles e deixa
capturas reais em `build/smoke_*.ppm`. É uma ferramenta de teste; o modo normal
de jogo, iniciado com `make run`, permanece inteiramente manual.
