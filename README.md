<h1 align="center"> Tetris </h1>
<h3 align="center"> Desenvolvimento do Jogo Tetris para o kit de desenvolvimento DE1-SoC para a disciplina de Sistemas Digitais </h3>
<h3 align="center"> Equipe: <a href="https://github.com/Danlrs">Daniel Lucas Rios da Silva</a>, <a href="https://github.com/ripe-glv">Filipe Carvalho Matos Galvão</a>, <a href="https://github.com/luanbsc">Luan Barbosa dos Santos Costa</a><br>
Docente: Ângelo Amâncio Duarte </h3>
<hr>

<div align="justify"> 
<h2> Resumo </h2>
O projeto em questão visa desenvolver um jogo semelhante ao tetris utilizando a linguagem de programação C, e a placa FPGA DE1-SoC para interação com o usuário, sendo que é utilizado o acelerômetro da placa para obter os movimentos desejados pelo usuário para as peças, assim como os botões da placa para permitir que o jogador interaja com o menu, pause o jogo, e gire as peças. Além disso, é utilizada sua saída VGA para apresentar a interface gráfica do jogo no monitor com a resolução de 640x480.
</div>

<h2> Sumário </h2>

- [Descrição dos Equipamentos e Softwares Utilizados](#descrição-dos-equipamentos-e-softwares-utilizados)
  - [Kit de Desenvolvimento DE1-SoC](#kit-de-desenvolvimento-de1-soc)
  - [Unidade de Processamento Gráfico Baseada em FPGA](#unidade-de-processamento-gráfico-baseada-em-fpga)
  - [Linguagem C](#linguagem-c)
  - [Compilador GNU (GCC)](#compilador-gnu-gcc)
  - [GDB (GNU Debugger)](#gdb-gnu-debugger)
  - [Visual Studio Code (VS Code)](#visual-studio-code-vs-code)
- [Funcionamento do Jogo](#funcionamento-do-jogo)
- [Interface e Controles](#interface-e-controles)
  - [Movimento das Peças - Acelerômetro](#movimento-das-peças---acelerômetro)
  - [Controle de Rotação e Interações com o Menu - Botões](#controle-de-rotação-e-interações-com-o-menu---botões)
  - [Exibição Gráfica - Saída VGA](#exibição-gráfica---saída-vga)
- [MakeFile](#makefile)
- [Conclusão](#conclusão)
- [Referências](#referências)

<h2 id="descrição-dos-equipamentos-e-softwares-utilizados">Descrição dos Equipamentos e Softwares Utilizados</h2>

<h3 id="kit-de-desenvolvimento-de1-soc">Kit de Desenvolvimento DE1-SoC</h3>
<p>O kit de desenvolvimento DE1-SoC da Terasic é uma plataforma projetada para facilitar o aprendizado e a experimentação com FPGAs. Ele é baseado no chip Cyclone V SoC da Intel, que integra um FPGA Cyclone V com um processador ARM dual-core, permitindo a interação entre hardware e software em uma única plataforma. Alguns dos principais componentes incluem:</p>
<ul>
  <li><strong>FPGA Cyclone V:</strong> Permite o desenvolvimento de lógica customizada em hardware.</li>
  <li><strong>Processador ARM Cortex-A9 dual-core:</strong> Para executar código de software e interagir com o FPGA.</li>
  <li><strong>Memória:</strong> Inclui SDRAM e memória flash para armazenamento de código e dados.</li>
  <li><strong>Acelerômetro:</strong> Utilizado neste projeto para capturar movimentos e controlar o jogo.</li>
  <li><strong>Saída VGA:</strong> Permite a exibição gráfica do jogo em um monitor.</li>
</ul>

<h3 id="unidade-de-processamento-gráfico-baseada-em-fpga">Unidade de Processamento Gráfico Baseada em FPGA</h3>
<p>O projeto utiliza uma unidade de processamento gráfico (GPU) desenvolvida para plataformas FPGA, projetada para jogos 2D. Esta GPU facilita o controle de sprites e a renderização de gráficos em tempo real com uma arquitetura baseada em sprites e suporte ao padrão VGA. A estrutura inclui módulos dedicados para sincronização de vídeo, gerenciamento de sprites e renderização de polígonos, permitindo uma exibição gráfica detalhada em resolução 640x480. Para o Tetris, aproveitamos o recurso de background desta GPU, que permite desenhar elementos de cenário de maneira otimizada, possibilitando a atualização eficiente dos blocos e da interface do jogo diretamente no hardware.</p>

<h3 id="linguagem-c">Linguagem C</h3>
<p>A linguagem C foi escolhida por sua eficiência e pelo controle direto que oferece sobre os recursos de hardware, o que é fundamental para o desenvolvimento de sistemas embarcados e jogos com interação direta com o hardware. Além disso, o C é amplamente suportado em plataformas de FPGA e permite o uso de bibliotecas específicas para controle dos periféricos.</p>

<h3 id="compilador-gnu-gcc">Compilador GNU (GCC)</h3>
<p>O GNU Compiler Collection (GCC) é um compilador popular para várias linguagens, incluindo C. Utilizamos o GCC para compilar o código C para o processador ARM da DE1-SoC, traduzindo o código-fonte em um binário executável que pode ser executado diretamente no hardware.</p>

<h3 id="gdb-gnu-debugger">GDB (GNU Debugger)</h3>
<p>O GNU Debugger (GDB) é uma ferramenta essencial para depuração, permitindo identificar e resolver problemas no código durante a execução. Com o GDB, é possível analisar o fluxo de execução, inspecionar variáveis e modificar o estado do programa em tempo real, facilitando o processo de depuração do jogo Tetris.</p>

<h3 id="visual-studio-code-vs-code">Visual Studio Code (VS Code)</h3>
<p>O Visual Studio Code é um editor de código leve, extensível e com suporte a várias linguagens e ferramentas de desenvolvimento. Para este projeto, o VS Code foi configurado com extensões para C/C++, integração com o GCC e GDB, além de suporte ao desenvolvimento remoto, permitindo a edição, compilação e depuração do código diretamente no ambiente da DE1-SoC.</p>

<h2 id="funcionamento-do-jogo">Funcionamento do Jogo</h2>
<p>O <strong>Tetris</strong> é um jogo em que o jogador organiza peças de formatos variados que caem de forma contínua na tela. O objetivo é movimentar e rotacionar essas peças para formar linhas horizontais completas, que são removidas, concedendo pontos ao jogador. O jogo termina quando as peças se acumulam e alcançam o topo da área de jogo.</p>

<h2 id="interface-e-controles">Interface e Controles</h2>
<p>Neste projeto, foram integrados vários periféricos da DE1-SoC para a interação com o jogo, conforme detalhado abaixo:</p>

<h3 id="movimento-das-peças---acelerômetro">1. Movimento das Peças - Acelerômetro</h3>
<p>O <strong>acelerômetro</strong> da DE1-SoC é utilizado para capturar os movimentos das peças no eixo horizontal (esquerda e direita) assim como no eixo vertical, para acelerar a queda da peça. Ao inclinar a placa levemente para a esquerda, direita, ou para baixo, as peças se movem de forma correspondente dentro da área de jogo. Este método de controle adiciona uma interatividade física ao jogo, tornando a experiência mais dinâmica e intuitiva.</p>

<h3 id="controle-de-rotação-e-interações-com-o-menu---botões">2. Controle de Rotação e Interações com o Menu - Botões</h3>
<p><strong>Botões</strong> da placa são utilizados para outras funcionalidades, como:</p>
<ul>
<li><strong>Rotação das peças</strong>: Pressionar um botão específico permite girar a peça atual, facilitando seu encaixe no tabuleiro.</li>
<li><strong>Interação com o menu</strong>: Um botão é reservado para navegar pelas opções de menu, como iniciar o jogo ou ajustar configurações.</li>
<li><strong>Pausa</strong>: Outro botão é dedicado à função de pausar e retomar o jogo conforme necessário.</li>
</ul>

<h3 id="exibição-gráfica---saída-vga">3. Exibição Gráfica - Saída VGA</h3>
<p>A interface gráfica do jogo é exibida através da <strong>saída VGA</strong>, permitindo que o jogo seja visualizado em um monitor externo. O monitor utiliza uma resolução de <strong>640x480</strong>, o que oferece uma experiência visual simples e eficiente para o Tetris. A área de jogo é clara e visível, com cores distintas entre as peças que estão caindo, e as que já estão posicionadas, e um layout de fácil compreensão.</p>

<h2 id="makefile">MakeFile</h2>
<p>O <strong>MakeFile</strong> é usado para gerenciar a compilação do código do jogo e facilitar o processo de geração do executável para a placa DE1-SoC. Esse arquivo contém comandos que automatizam a construção do projeto, configurando e compilando o código-fonte, e gerando o binário final a ser carregado na placa.</p>

<h2 id="conclusão">Conclusão</h2>
<p>Este projeto demonstrou a integração de hardware e software para criar uma versão do jogo Tetris com uma experiência física de controle, aproveitando o acelerômetro e botões do kit DE1-SoC. O projeto enfatiza a viabilidade de construir uma aplicação interativa com recursos de programação embarcada e controles alternativos.</p>

<h2 id="referências">Referências</h2>
<p>ALVES, Gabriel B. S.; DIAS, Anfranserai M.; SARINHO, Victor T. Desenvolvimento de uma Arquitetura Baseada em Sprites para Criação de Jogos 2D em Ambientes Reconfiguráveis utilizando dispositivos FPGA. Universidade Estadual de Feira de Santana, Feira de Santana, Bahia, Brasil.</p>

