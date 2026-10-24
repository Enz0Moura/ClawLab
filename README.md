# LaClaw Lab

**Português (Brasil)** | [English](README.en.md)

Biblioteca de computação gráfica com OOP em **C11** e simulador 3D de uma máquina
de prêmios, usando **OpenGL e freeglut**. O braço tem base, ombro, cotovelo, punho
e dois dedos articulados. Controle as juntas, capture três alienígenas e solte-os na
bandeja. A captura é determinística; o modo escolhido define o tempo, a intensidade
dos quiques, a variação das posições iniciais dos aliens e o tamanho da bandeja.

O visual é inspirado na máquina do **Pizza Planet, de Toy Story**.

Projeto para o **cenário 3 — Simulador de Braço Robótico**, CG 2026/2,
CEFET/RJ, UnED Petrópolis. IK não foi implementada.

A arquitetura segue os padrões de `Geometry`, base incorporada, métodos com
`self` e fábricas de
[Enz0Moura/primeiro-trabalho-cg](https://github.com/Enz0Moura/primeiro-trabalho-cg/tree/c12a40e4254ea511ad8906883b9ca91b4a429dc0),
com matemática 3D, árvore de cena, destrutores e propriedade de memória explícita.

## Compilar e executar

Dependências: compilador C11, Make, headers/bibliotecas OpenGL e freeglut.
No CachyOS/Arch: `sudo pacman -S --needed base-devel freeglut libglvnd`.
É necessário um display gráfico para abrir as janelas; os testes não abrem janela.

```sh
make lib       # build/libcg.a, independente da aplicação
make demo      # compila build/library_demo
make example   # executa o exemplo independente
make run       # compila e executa build/prize_machine
make test      # matemática, estados, trajetórias e falhas de alocação
make sanitize  # os mesmos testes com Clang, ASan e UBSan
make smoke     # sessão gráfica automática completa; requer display
make clean     # remove build/
```

`make` compila os dois executáveis. O simulador e o exemplo vinculam o mesmo
`build/libcg.a`. Escolha um modo no menu com o mouse ou **1–4**.
Para jogar: clique num prêmio; escolha a junta com **1–4**;
use **←/→** ou **+/−**; **C** fecha, **O** abre, **H** mostra a ajuda.
Botão direito e roda controlam a câmera. **R** reinicia; **D** mostra a depuração.
Arraste com a roda pressionada para deslocar a visão. **F** alterna a câmera
livre: botão direito olha ao redor e a roda avança ou recua. **Home** restaura a câmera.

## Modos de jogo

| Modo | Tempo | Bandeja (largura × profundidade) | Posições iniciais | Colisões |
| --- | --- | --- | --- | --- |
| Sandbox | Sem limite | 2,50 × 1,60 | Pequena variação angular, distância constante | Quiques suaves para praticar |
| Fácil | 4 minutos | 2,35 × 1,40 | Pequena variação angular, distância constante | Quiques leves e maior amortecimento |
| Médio | 3 minutos | 2,20 × 1,20 | Mais variação angular e três distâncias possíveis | Quiques moderados |
| Difícil | 2 minutos | 2,05 × 1,00 | Maior variação angular e sete distâncias possíveis | Quiques fortes e menor amortecimento |

As medidas usam unidades do cenário. A bandeja diminui em largura e profundidade,
mantendo o centro e a altura. Mesmo no difícil, cabem três aliens separados;
a validação dos spawns exige uma rota de entrega com margem nas bordas.
O reset mantém o tamanho do modo atual, e a pausa preserva a partida.

Cada nova partida ou reset sorteia as posições e embaralha as cores entre elas.
Os aliens começam apoiados no piso, separados entre si e fora da bandeja e dos
obstáculos. O sorteio valida uma rota de aproximação, captura, retirada e transporte
para cada alien. Pausar e continuar preserva o layout atual.

Aliens soltos quicam no piso, na cabine e entre si. A entrega conta apenas quando
o alien para diretamente na bandeja, inteiramente dentro dela. **P**, **Esc** ou o
botão **P PAUSA** abre a pausa e congela a simulação e o cronômetro. **Continuar**
retoma exatamente a partida atual, preservando câmera, garra, aliens e pontuação.
Na pausa, **Resetar partida** reinicia o modo atual e **Voltar aos modos** encerra
a partida para escolher outro modo. **R** também reinicia durante o jogo.
A ajuda congela o tempo. Vitória ou tempo esgotado encerram a rodada.

## Documentação

Veja o [manual do usuário](docs/MANUAL_DO_USUARIO.md).

Validado em CachyOS Linux, freeglut 3.8.0 e OpenGL de compatibilidade 4.6/NVIDIA.
Usa AABBs conservadoras, gravidade, colisões com quique e atrito simplificado numa
região limitada da cabine. Não implementa autocolisão do braço nem rotação física dos prêmios.
