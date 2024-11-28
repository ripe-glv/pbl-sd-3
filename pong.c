#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <errno.h>   // Para tratar erros de read()

// Endereços usados para mapeamento e funcionalidades dos botões
// #define LW_BRIDGE_BASE 0xFF200000
// #define LW_BRIDGE_SPAN 0x00005000
// #define KEY_BASE 0x00000050
// #define BUTTON_0_MASK 0x01
// #define BUTTON_1_MASK 0x02
// #define BUTTON_2_MASK 0x04
// #define BUTTON_3_MASK 0x08

// Tamanho do tabuleiro do jogo
#define HEIGHT 20
#define WIDTH 40

// Endereços usados para mapeamento e funcionalidades do acelerometro
typedef int bool;
#define true 1
#define false 0
#define SYSMGR_BASE 0xFFD08000
#define SYSMGR_SPAN 0x00000800
// #define SYSMGR_GENERALIO7 ((volatile unsigned int *) 0xffd0849C)
// #define SYSMGR_GENERALIO8 ((volatile unsigned int *) 0xffd084A0)
// #define SYSMGR_I2C0USEFPGA ((volatile unsigned int *) 0xffd08704)
#define I2C0_BASE 0xFFC04000
#define I2C0_SPAN 0x00000100
// #define ADXL345_REG_DATAX0 0x32
// #define ADXL345_REG_DATAX1 0x33
// #define ADXL345_REG_DATAY0 0x34
// #define ADXL345_REG_DATAY1 0x35
#define ADXL345_REG_DATA_FORMAT 0x31
#define XL345_RANGE_16G 0x03
#define XL345_FULL_RESOLUTION 0x08
#define ADXL345_REG_BW_RATE 0x2C
#define XL345_RATE_200 0x0b
#define ADXL345_REG_THRESH_ACT 0x24
#define ADXL345_REG_THRESH_INACT 0x25
#define ADXL345_REG_TIME_INACT 0x26
#define ADXL345_REG_ACT_INACT_CTL 0x27
#define ADXL345_REG_INT_ENABLE 0x2E
#define XL345_ACTIVITY 0x10
#define XL345_INACTIVITY 0x08
#define ADXL345_REG_POWER_CTL 0x2D
#define XL345_MEASURE 0x08
#define XL345_STANDBY 0x00
#define ADXL345_REG_INT_SOURCE 0x30
#define ADXL345_REG_OFSX 0x1E
#define ADXL345_REG_OFSY 0x1F
#define ADXL345_REG_OFSZ 0x20
#define XL345_RATE_100 0x0a
#define XL345_DATAREADY 0x80
#define ROUNDED_DIVISION(n, d) (((n < 0) ^ (d < 0)) ? ((n - d/2)/d) : ((n + d/2)/d))


// Variaveis que representam os registradores do acelerometro
void * I2C0_virtual;
void * sysmgr_virtual;
volatile int * ic_con;
volatile int * ic_tar;
volatile int * ic_fs_scl_hcnt; 
volatile int * ic_fs_scl_lcnt;
volatile int * ic_enable;
volatile int * ic_enable_status;
volatile int * ic_data_cmd;
volatile int * ic_rxflr;

extern void mapm();
extern void draw_box(int posicao, int cor);
extern void draw_text(int posicao, int cor);
extern void draw_polygon(int forma, int cor, int tamanho, int y, int x);
extern void draw_sprite(int reg, int sp, int x, int y, int offset);
extern void limpar_tela();
extern int button_pressed();

#define HEIGHT 20
#define WIDTH 40

// Variáveis globais
char board[HEIGHT][WIDTH];
int ballX, ballY;       // Posição da bola
int ballDirX = 1, ballDirY = 1; // Direção da bola
int paddle1Y = HEIGHT / 2, paddle2Y = HEIGHT / 2; // Posições das raquetes
int paddleSize = 4; // Tamanho da raquete
int score1 = 0, score2 = 0;
int opt = 0;
int estado = 1; //estado = 0 - jogando, estado = 1 - menu, estado = 2 - game over
int first_loop = 1;
int pause2 = 0;

// Função para mapear a memória
void *map_physical_memory(off_t base, size_t span) {
    int fd;
    void *virtual_base;

    // Abre o arquivo de memória
    if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) {
        printf("ERROR: could not open \"/dev/mem\"...\n");
        return NULL;
    }

    // Mapeia a memória física para o espaço de endereçamento do processo
    virtual_base = mmap(NULL, span, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, base);
    if (virtual_base == MAP_FAILED) {
        printf("ERROR: mmap() failed...\n");
        close(fd);
        return NULL;
    }

    close(fd);
    return virtual_base;
}

void ADXL345_REG_READ(uint8_t address, uint8_t *value){
    *ic_data_cmd = address + 0x400;
    *ic_data_cmd = 0x100;
    while (*ic_rxflr == 0){}
    *value = *ic_data_cmd;
}

void ADXL345_REG_WRITE(uint8_t address, uint8_t value){
 
    // Envia endereço do registrador (+0x400 para enviar sinal de START)
    *ic_data_cmd = address + 0x400;

    // Envia o valor
    *ic_data_cmd = value;
}

bool ADXL345_WasActivityUpdated(){
    bool bReady = false;
    uint8_t data8;

    ADXL345_REG_READ(ADXL345_REG_INT_SOURCE,&data8);
    if (data8 & XL345_ACTIVITY)
    bReady = true;

    return bReady;
}

// Inicia o acelerometro
void ADXL345_Init(){

    ADXL345_REG_WRITE(ADXL345_REG_DATA_FORMAT, XL345_RANGE_16G | XL345_FULL_RESOLUTION);

    ADXL345_REG_WRITE(ADXL345_REG_BW_RATE, XL345_RATE_200);

    ADXL345_REG_WRITE(ADXL345_REG_THRESH_ACT, 0x04);
    ADXL345_REG_WRITE(ADXL345_REG_THRESH_INACT, 0x02);
    ADXL345_REG_WRITE(ADXL345_REG_TIME_INACT, 0x02);
    ADXL345_REG_WRITE(ADXL345_REG_ACT_INACT_CTL, 0xFF);
    ADXL345_REG_WRITE(ADXL345_REG_INT_ENABLE, XL345_ACTIVITY | XL345_INACTIVITY );

    ADXL345_REG_WRITE(ADXL345_REG_POWER_CTL, XL345_STANDBY);

    ADXL345_REG_WRITE(ADXL345_REG_POWER_CTL, XL345_MEASURE);

    ADXL345_Calibrate();
}

void ADXL345_REG_MULTI_READ(uint8_t address, uint8_t values[], uint8_t len){

    // Envia endereço do registrador (+0x400 para enviar sinal de START)
    *ic_data_cmd = address + 0x400;

    // Envia o sinal de leitura len vezes
    int i;
    for (i=0;i<len;i++)
        *ic_data_cmd = 0x100;

    // Faz a leitura dos bytes
    int nth_byte=0;
    while (len){
        if ((*ic_rxflr) > 0){
            values[nth_byte] = *ic_data_cmd;
            nth_byte++;
            len--;
        }
    }
}

//  Lê as coordenadas dos eixos do acelerometro
void ADXL345_XYZ_Read(int16_t szData16[3]){
    uint8_t szData8[6];
    ADXL345_REG_MULTI_READ(0x32, (uint8_t *)&szData8, sizeof(szData8));

    szData16[0] = (szData8[1] << 8) | szData8[0]; 
    szData16[1] = (szData8[3] << 8) | szData8[2];
    szData16[2] = (szData8[5] << 8) | szData8[4];
}

//  Faz a configuracao inicial do IC20
void initConfigIC20(){
    ic_con = (int *) (I2C0_virtual + 0x0);
    ic_tar = (int *) (I2C0_virtual + 0x4);
    ic_fs_scl_hcnt = (int *) (I2C0_virtual + 0x1C);
    ic_fs_scl_lcnt = (int *) (I2C0_virtual + 0x20);
    ic_enable = (int *) (I2C0_virtual + 0x6C);
    ic_enable_status = (int *) (I2C0_virtual + 0x9C);
    ic_data_cmd = (int *) (I2C0_virtual + 0x10);
    ic_rxflr = (int *) (I2C0_virtual + 0x78);

    *ic_enable = 2;

    while(((*ic_enable_status)&0x1) == 1){}

    *ic_con = 0x65;
    *ic_tar = 0x53;
    *ic_fs_scl_hcnt = 90;
    *ic_fs_scl_lcnt = 160;
    *ic_enable = 1;

    while(((*ic_enable_status)&0x1) == 0){}
}

bool ADXL345_IsDataReady(){
    bool bReady = false;
    uint8_t data8;

    ADXL345_REG_READ(ADXL345_REG_INT_SOURCE,&data8);
    if (data8 & XL345_DATAREADY)
        bReady = true;

    return bReady;
}

// Calibra os valores do acelerometro
void ADXL345_Calibrate(){

    int average_x = 0;
    int average_y = 0;
    int average_z = 0;
    int16_t XYZ[3];
    int8_t offset_x;
    int8_t offset_y;
    int8_t offset_z;

    // Para a medida
    ADXL345_REG_WRITE(ADXL345_REG_POWER_CTL, XL345_STANDBY);

    // Pega os offsets atuais
    ADXL345_REG_READ(ADXL345_REG_OFSX, (uint8_t *)&offset_x);
    ADXL345_REG_READ(ADXL345_REG_OFSY, (uint8_t *)&offset_y);
    ADXL345_REG_READ(ADXL345_REG_OFSZ, (uint8_t *)&offset_z);

    // Usa 100Hz para calibração. Salva a taxa atual.
    uint8_t saved_bw;
    ADXL345_REG_READ(ADXL345_REG_BW_RATE, &saved_bw);
    ADXL345_REG_WRITE(ADXL345_REG_BW_RATE, XL345_RATE_100);

    // Usa alcance de 16g, resolução completa. Salva o formato atual.
    uint8_t saved_dataformat;
    ADXL345_REG_READ(ADXL345_REG_DATA_FORMAT, &saved_dataformat);
    ADXL345_REG_WRITE(ADXL345_REG_DATA_FORMAT, XL345_RANGE_16G | XL345_FULL_RESOLUTION);

    // Inicia a medida
    ADXL345_REG_WRITE(ADXL345_REG_POWER_CTL, XL345_MEASURE);

    // Pega a média de acelerações de x, y e z em 32 amostras (LSB 3.9 mg)
    int i = 0;
    while (i < 32){
    // Detalhe: use DATA_READY aqui, não pode usar ACTIVITY porque o tabuleiro é fixo.
        if (ADXL345_IsDataReady()){
            ADXL345_XYZ_Read(XYZ);
            average_x += XYZ[0];
            average_y += XYZ[1];
            average_z += XYZ[2];
            i++;
        }
    }

    average_x = ROUNDED_DIVISION(average_x, 32);
    average_y = ROUNDED_DIVISION(average_y, 32);
    average_z = ROUNDED_DIVISION(average_z, 32);

    // Para a medida
    ADXL345_REG_WRITE(ADXL345_REG_POWER_CTL, XL345_STANDBY); 

    // Calcula os offsets (LSB 15.6 mg)
    offset_x += ROUNDED_DIVISION(0-average_x, 4);
    offset_y += ROUNDED_DIVISION(0-average_y, 4);
    offset_z += ROUNDED_DIVISION(256-average_z, 4);

    // Define o offset do registrador
    ADXL345_REG_WRITE(ADXL345_REG_OFSX, offset_x);
    ADXL345_REG_WRITE(ADXL345_REG_OFSY, offset_y);
    ADXL345_REG_WRITE(ADXL345_REG_OFSZ, offset_z);

    // Restaura a taxa original bw
    ADXL345_REG_WRITE(ADXL345_REG_BW_RATE, saved_bw);

    // Restaura o formato original dos dados
    ADXL345_REG_WRITE(ADXL345_REG_DATA_FORMAT, saved_dataformat);

    // Inicia a medida
    ADXL345_REG_WRITE(ADXL345_REG_POWER_CTL, XL345_MEASURE);
}

void limpa_sprites(){
    for (int i = 0; i < 100; i++){
        draw_sprite(i, 0, 0, 0, 0);
    }
}

void limpar_tudo(){
    limpar_tela();
    limpa_sprites();
    draw_polygon(1, 0, 0, 0, 0);
}

// Função para inicializar o tabuleiro
void initBoard() {
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            if (j == 0 || j == WIDTH - 1) // Bordas laterais
                board[i][j] = '|';
            else
                board[i][j] = ' ';
        }
    }

    ballX = WIDTH / 2;
    ballY = HEIGHT / 2;
}

// Desenha uma linha vertical
void draw_vertical(int pos, int color, int tamanho) {
    for (int i = 0; i < tamanho; i++){
        draw_box(pos + i*160, color);
    }
}

// Desenha uma linha horizontal
void draw_horizontal(int pos, int color, int tamanho) {
    for (int i = 0; i < tamanho; i++){
        draw_box(pos + i*2, color);
    }
}

// Desenha uma linha fina vertical
void draw_vertical_text(int pos, int color, int tamanho) {
    for (int i = 0; i < tamanho; i++){
        draw_text(pos + i*80, color);
    }
}

// Desenha uma linha fina horizontal
void draw_horizontal_text(int pos, int color, int tamanho) {
    for (int i = 0; i < tamanho; i++){
        draw_text(pos + i, color);
    }
}

// Função do menu inicial
void draw_menu(){
    //P
    draw_horizontal(578, 0b110110110, 5);
    draw_vertical(738, 0b110110110, 4);
    draw_vertical(746, 0b110110110, 1);
    draw_horizontal(900, 0b110110110, 4);

    //O
    draw_horizontal(590, 0b110110110, 5);
    draw_vertical(750, 0b110110110, 4);
    draw_vertical(758, 0b110110110, 4);
    draw_horizontal(1232, 0b110110110, 3);

    //N
    draw_vertical(602, 0b110110110, 5);
    draw_box(764, 0b110110110);
    draw_box(926, 0b110110110);
    draw_box(1088, 0b110110110);
    draw_vertical(610, 0b110110110, 5);

    //G
    draw_horizontal(614, 0b110110110, 5);
    draw_vertical(774, 0b110110110, 4);
    draw_horizontal(1256, 0b110110110, 4);
    draw_vertical(942, 0b110110110, 2);
    draw_box(940, 0b110110110);

    if(opt == 0){
            // Seletor em Jogar

            // Desenha a palavra Jogar
            // Desenha a letra J
            draw_vertical_text(2588, 0b010111000, 2);
            draw_horizontal_text(2669, 0b010111000, 2);
            draw_vertical_text(2350, 0b010111000, 4);

            // Desenha a letra O
            draw_horizontal_text(2352, 0b010111000, 3);
            draw_vertical_text(2432, 0b010111000, 3);
            draw_vertical_text(2434, 0b010111000, 3);
            draw_horizontal_text(2672, 0b010111000, 3);

            // Desenha a letra G
            draw_horizontal_text(2356, 0b010111000, 3);
            draw_vertical_text(2436, 0b010111000, 3);
            draw_horizontal_text(2676, 0b010111000, 3);
            draw_text(2598, 0b010111000);

            // Desenha a letra A
            draw_horizontal_text(2360, 0b010111000, 3);
            draw_vertical_text(2440, 0b010111000, 4);
            draw_vertical_text(2442, 0b010111000, 4);
            draw_text(2521, 0b010111000);

            // Desenha a letra R
            draw_horizontal_text(2364, 0b010111000, 2);
            draw_vertical_text(2444, 0b010111000, 4);
            draw_text(2446, 0b010111000);
            draw_text(2525, 0b010111000);
            draw_vertical_text(2606, 0b010111000, 2);

            // Desenha a palavra Sair
            // Desenha a letra S
            draw_horizontal_text(2909, 0b111111111, 2);
            draw_text(2988, 0b111111111);
            draw_horizontal_text(3068, 0b111111111, 3);
            draw_text(3150, 0b111111111);
            draw_horizontal_text(3228, 0b111111111, 2);

            // Desenha a letra A
            draw_horizontal_text(2912, 0b111111111, 3);
            draw_vertical_text(2992, 0b111111111, 4);
            draw_vertical_text(2994, 0b111111111, 4);
            draw_text(3073, 0b111111111);

            // Desenha a letra I
            draw_horizontal_text(2916, 0b111111111, 3);
            draw_vertical_text(2997, 0b111111111, 3);
            draw_horizontal_text(3236, 0b111111111, 3);

            // Desenha a letra R
            draw_horizontal_text(2920, 0b111111111, 2);
            draw_vertical_text(3000, 0b111111111, 4);
            draw_text(3002, 0b111111111);
            draw_text(3081, 0b111111111);
            draw_vertical_text(3162, 0b111111111, 2);

        } else if(opt == 1){
            // Seletor em Sair

            // Desenha a palavra Jogar
            // Desenha a letra J
            draw_vertical_text(2588, 0b111111111, 2);
            draw_horizontal_text(2669, 0b111111111, 2);
            draw_vertical_text(2350, 0b111111111, 4);

            // Desenha a letra O
            draw_horizontal_text(2352, 0b111111111, 3);
            draw_vertical_text(2432, 0b111111111, 3);
            draw_vertical_text(2434, 0b111111111, 3);
            draw_horizontal_text(2672, 0b111111111, 3);

            // Desenha a letra G
            draw_horizontal_text(2356, 0b111111111, 3);
            draw_vertical_text(2436, 0b111111111, 3);
            draw_horizontal_text(2676, 0b111111111, 3);
            draw_text(2598, 0b111111111);

            // Desenha a letra A
            draw_horizontal_text(2360, 0b111111111, 3);
            draw_vertical_text(2440, 0b111111111, 4);
            draw_vertical_text(2442, 0b111111111, 4);
            draw_text(2521, 0b111111111);

            // Desenha a letra R
            draw_horizontal_text(2364, 0b111111111, 2);
            draw_vertical_text(2444, 0b111111111, 4);
            draw_text(2446, 0b111111111);
            draw_text(2525, 0b111111111);
            draw_vertical_text(2606, 0b111111111, 2);

            // Desenha a palavra Sair
            // Desenha a letra S
            draw_horizontal_text(2909, 0b010111000, 2);
            draw_text(2988, 0b010111000);
            draw_horizontal_text(3068, 0b010111000, 3);
            draw_text(3150, 0b010111000);
            draw_horizontal_text(3228, 0b010111000, 2);

            // Desenha a letra A
            draw_horizontal_text(2912, 0b010111000, 3);
            draw_vertical_text(2992, 0b010111000, 4);
            draw_vertical_text(2994, 0b010111000, 4);
            draw_text(3073, 0b010111000);

            // Desenha a letra I
            draw_horizontal_text(2916, 0b010111000, 3);
            draw_vertical_text(2997, 0b010111000, 3);
            draw_horizontal_text(3236, 0b010111000, 3);

            // Desenha a letra R
            draw_horizontal_text(2920, 0b010111000, 2);
            draw_vertical_text(3000, 0b010111000, 4);
            draw_text(3002, 0b010111000);
            draw_text(3081, 0b010111000);
            draw_vertical_text(3162, 0b010111000, 2);

        }


}

void drawPaddles() {
    int position; // Variável para calcular a posição na tela

    // Desenha a raquete do jogador 1
    for (int i = 0; i < HEIGHT; i++) {
        position = (i * 160) + 2; // Posição fixa na coluna 1 (coluna 2 na tela)
        if (i >= paddle1Y - paddleSize / 2 && i <= paddle1Y + paddleSize / 2) {
            board[i][1] = "#";
            draw_box(position, 0b010111010); // Cinza para a raquete
        } else {
            board[i][1] = " ";
            draw_box(position, 0b000000000); // Preto para espaços vazios
        }
    }

    // Desenha a raquete do jogador 2
    for (int i = 0; i < HEIGHT; i++) {
        position = (i * 160) + (WIDTH - 2) * 2; // Posição fixa na penúltima coluna
        if (i >= paddle2Y - paddleSize / 2 && i <= paddle2Y + paddleSize / 2) {
            board[i][WIDTH - 2] = "#";
            draw_box(position, 0b111000000); // Cinza para a raquete
        } else {
            board[i][WIDTH - 2] = " ";
            draw_box(position, 0b000000000); // Preto para espaços vazios
        }
    }
}


// Função para atualizar a posição da bola
void updateBall() {
    // Apaga a bola da posição atual
    board[ballY][ballX] = ' ';

    // Atualiza a posição da bola
    ballX += ballDirX;
    ballY += ballDirY;

    // Verificar colisão com as paredes superior e inferior
    if (ballY <= 0) {
        ballY = 0;
        ballDirY = -ballDirY;
    } else if (ballY >= HEIGHT - 1) {
        ballY = HEIGHT - 1;
        ballDirY = -ballDirY;
    }

    // Verificar colisão com as raquetes
    if (ballX == 2 && ballY >= paddle1Y - paddleSize / 2 && ballY <= paddle1Y + paddleSize / 2) {
        ballX = 2; // Reajusta a posição para evitar sobreposição
        ballDirX = -ballDirX;
    } else if (ballX == WIDTH - 3 && ballY >= paddle2Y - paddleSize / 2 && ballY <= paddle2Y + paddleSize / 2) {
        ballX = WIDTH - 3; // Reajusta a posição para evitar sobreposição
        ballDirX = -ballDirX;
    }

    // Garante que a bola não ultrapasse as pontas da raquete
    if ((ballX == WIDTH - 2 || ballX == 1) && (ballY < paddle1Y - paddleSize / 2 || ballY > paddle1Y + paddleSize / 2) && 
        (ballY < paddle2Y - paddleSize / 2 || ballY > paddle2Y + paddleSize / 2)) {
        ballX = (ballDirX > 0) ? WIDTH - 1 : 0;
    }

    // Verificar se há pontuação
    if (ballX <= 0) {
        score2++;
        ballX = WIDTH / 2;
        ballY = HEIGHT / 2;
        ballDirX = 1;

        pontos();
    } else if (ballX >= WIDTH - 1) {
        score1++;
        ballX = WIDTH / 2;
        ballY = HEIGHT / 2;
        ballDirX = -1;

        pontos();
    }

    // Atualiza a posição da bola no tabuleiro
    board[ballY][ballX] = 'O';

    draw_ball();
}

void draw_ball(){
    int position; // Variável para calcular a posição na tela
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            // Calcula a posição na tela baseada na linha e coluna
            position = (i * 160) + (j * 2);
            
            if (board[i][j] == 'O') { // Bola
                draw_box(position, 0b110110110); // Cor cinza
            }else if (board[i][j] == ' '){ // Espaços vazios
                draw_box(position, 0b000000000); // Preto
            }
        }
    }
}

void pontos(){
    draw_horizontal(3474, 0b000000000, 11);
    draw_horizontal(3554, 0b000000000, 11);
    draw_horizontal(3634, 0b000000000, 11);
    draw_horizontal(3714, 0b000000000, 11);
    draw_horizontal(3794, 0b000000000, 11);

    //Pontos P1
    Desenha_number(3474, score1);
    //-
    draw_horizontal_text(3638, 0b111111111, 3);
    //Pontos P2
    Desenha_number(3482, score2);

    drawPaddles();

    if(score1 > score2){
        draw_polygon(1, 0b000111000, 1, 400, 283);
    }else if(score2 > score1){
        draw_polygon(1, 0b000111000, 1, 400, 346);
    }
}
void Desenha_number(int posicao, int numero){
    switch (numero){

        case 0:
            // Desenha o 0 (ajustar posição)
            draw_text(posicao + 1, 0b111111111);
            draw_vertical_text(posicao + 80, 0b111111111, 3);
            draw_vertical_text(posicao + 82, 0b111111111, 3);
            draw_text(posicao + 321, 0b111111111);
            break;

        case 1:

            // Desenha o 1 (ajustar posição)
            draw_vertical_text(posicao + 1, 0b111111111, 5);
            draw_text(posicao + 80  , 0b111111111);
            break;

        case 2:

            // Desenha o 2 (ajustar posição)
            draw_horizontal_text(posicao, 0b111111111, 3);
            draw_vertical_text(posicao + 82, 0b111111111, 2);
            draw_horizontal_text(posicao + 160, 0b111111111, 2);
            draw_vertical_text(posicao + 240, 0b111111111, 2);
            draw_horizontal_text(posicao + 321, 0b111111111, 2);
            break;

            draw_horizontal_text(3515, 0b111111111, 3);
            draw_vertical_text(3597, 0b111111111, 2);
            draw_horizontal_text(3675, 0b111111111, 2);
            draw_vertical_text(3755, 0b111111111, 2);
            draw_horizontal_text(3836, 0b111111111, 2);

        case 3:

            // Deseha o 3 (ajustar posição)
            draw_text(posicao, 0b111111111);
            draw_text(posicao + 1, 0b111111111);
            draw_text(posicao + 82, 0b111111111);
            draw_text(posicao + 161, 0b111111111);
            draw_text(posicao + 320, 0b111111111);
            draw_text(posicao + 321, 0b111111111);
            draw_text(posicao + 242, 0b111111111); 
            break;

        case 4:

            // Desenha o 4 (ajustar posição)
            draw_vertical_text(posicao, 0b111111111, 3);
            draw_text(posicao + 161, 0b111111111);
            draw_vertical_text(posicao + 2, 0b111111111, 5);
            break;

        case 5:
            // Desenha o 5 (ajustar posição)
            draw_horizontal_text(posicao, 0b111111111, 3);
            draw_horizontal_text(posicao + 80, 0b111111111, 1);
            draw_horizontal_text(posicao + 160, 0b111111111, 3);
            draw_horizontal_text(posicao + 242, 0b111111111, 1);
            draw_horizontal_text(posicao + 320, 0b111111111, 3);
            break;

        case 6:
            // Desenha o 6 (ajustar posição)
            draw_horizontal_text(posicao + 1, 0b111111111, 2);
            draw_vertical_text(posicao, 0b111111111, 5);
            draw_horizontal_text(posicao + 321, 0b111111111, 2);
            draw_vertical_text(posicao + 162, 0b111111111, 2);
            draw_text(posicao + 161, 0b111111111);
            break;

        case 7:

            // Desenha o 7 inclinado 
            draw_horizontal_text(posicao, 0b111111111, 3);
            draw_vertical_text(posicao + 82, 0b111111111, 1);
            draw_vertical_text(posicao + 161, 0b111111111, 3);
            break;

        case 8:

            // Desenha o 8
            draw_horizontal_text(posicao, 0b111111111, 3);
            draw_vertical_text(posicao + 80, 0b111111111, 3);
            draw_text(posicao + 161, 0b111111111);
            draw_vertical_text(posicao + 82, 0b111111111, 3);
            draw_horizontal_text(posicao + 320, 0b111111111, 3);
            break;
        
        case 9:

            // Desenha o 9
            draw_horizontal_text(posicao, 0b111111111, 3);
            draw_text(posicao + 80, 0b111111111);
            draw_horizontal_text(posicao + 160, 0b111111111, 2);
            draw_vertical_text(posicao + 2, 0b111111111, 5);
            break;

        default:
            break;    
    }

}

// Função para mostrar o tabuleiro
void displayBoard() {
    draw_horizontal(3200, 0b111111111, 40);


    //P (p1)
    draw_vertical_text(3441, 0b111111111, 5);
    draw_horizontal_text(3442, 0b111111111, 2);
    draw_vertical_text(3523, 0b111111111, 2);
    draw_text(3602, 0b111111111);

    //1
    draw_vertical_text(3446, 0b111111111, 5);
    draw_text(3525, 0b111111111);


    //P (p2)
    draw_vertical_text(3511, 0b111111111, 5);
    draw_horizontal_text(3512, 0b111111111, 2);
    draw_vertical_text(3593, 0b111111111, 2);
    draw_text(3672, 0b111111111);

    //2
    draw_horizontal_text(3515, 0b111111111, 3);
    draw_vertical_text(3597, 0b111111111, 2);
    draw_horizontal_text(3675, 0b111111111, 2);
    draw_vertical_text(3755, 0b111111111, 2);
    draw_horizontal_text(3836, 0b111111111, 2);

    int position; // Variável para calcular a posição na tela
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            // Calcula a posição na tela baseada na linha e coluna
            position = (i * 160) + (j * 2);

            // Determina a cor baseada no conteúdo da matriz do tabuleiro
            if (board[i][j] == '|') { // Bordas laterais
                draw_box(position, 0b111111111); // Cor branca
            } else if (board[i][j] == '#') { // Raquetes
                draw_box(position, 0b101101101); // Cor cinza
            } else if(board[i][j] == ' '){ // Espaços vazios
                draw_box(position, 0b000000000); // Preto
            }
        }
    }
}

// Lê eventos do mouse e move a raquete do jogador 2
void handleMouse(int mouseFile) {
    char buffer[3]; // Buffer para armazenar o evento do mouse
    int bytesRead = read(mouseFile, buffer, 3);

    // Verifica se houve movimento
    if (bytesRead > 0) {
        signed char dy = (signed char)buffer[2]; // Converte explicitamente para signed char
        if (dy > 0 && paddle2Y > paddleSize / 2) { // Movimento para cima
            paddle2Y--;
            drawPaddles();
        } else if (dy < 0 && paddle2Y < HEIGHT - paddleSize / 2) { // Movimento para baixo
            paddle2Y++;
            drawPaddles();
        }
        
    } else if (bytesRead == -1 && errno != EAGAIN) {
        perror("Erro ao ler evento do mouse");
    }
}

void jogo_pause(){

    // If que checa a opção a ser inserida no menu de pausa e muda o indicador (>)
    if(opt == 0){

        // Desenhando símbolo de play
        draw_vertical_text(2353, 0b010111000, 7);
        draw_text(2434, 0b010111000);
        draw_text(2515, 0b010111000);
        draw_text(2596, 0b010111000);
        draw_text(2675, 0b010111000);
        draw_text(2754, 0b010111000);

        // Desenhando X
        draw_text(2440, 0b111111111);
        draw_text(2521, 0b111111111);
        draw_text(2602, 0b111111111);
        draw_text(2681, 0b111111111);
        draw_text(2760, 0b111111111);
        draw_text(2683, 0b111111111);
        draw_text(2764, 0b111111111);
        draw_text(2523, 0b111111111);
        draw_text(2444, 0b111111111);

    } else if(opt == 1){

        // Desenhando símbolo de play
        draw_vertical_text(2353, 0b111111111, 7);
        draw_text(2434, 0b111111111);
        draw_text(2515, 0b111111111);
        draw_text(2596, 0b111111111);
        draw_text(2675, 0b111111111);
        draw_text(2754, 0b111111111);

        // Desenhando X
        draw_text(2440, 0b000000111);
        draw_text(2521, 0b000000111);
        draw_text(2602, 0b000000111);
        draw_text(2681, 0b000000111);
        draw_text(2760, 0b000000111);
        draw_text(2683, 0b000000111);
        draw_text(2764, 0b000000111);
        draw_text(2523, 0b000000111);
        draw_text(2444, 0b000000111);

    }
}

// Função principal
int main() {

    // Abrir o dispositivo do mouse
    int mouseFile = open("/dev/input/mouse0", O_RDONLY | O_NONBLOCK);
    if (mouseFile == -1) {
        perror("Erro ao abrir dispositivo do mouse");
        return 1;
    }

     // Mapeia a memoria do I2CO e o sysmgr
    I2C0_virtual = map_physical_memory(I2C0_BASE, I2C0_SPAN);
    sysmgr_virtual = map_physical_memory(SYSMGR_BASE, SYSMGR_SPAN);

    uint8_t devid;
    int16_t mg_per_lsb = 4;
    int16_t XYZ[3];

    initConfigIC20();

    ADXL345_REG_READ(0x00, &devid);

    // Checa o endereco do device (acelerometro)
    if (devid == 0xE5){
        ADXL345_Init();
        srand(time(NULL));

        // Mapeia a memória
        mapm();
        limpar_tudo();
        
        int button_value = 0;
        initBoard();
        while (1) {
            int button = button_pressed();
            if (button != 15) {
                button_value = button;
            }

            if(estado == 1){
                draw_menu();
            }else if(estado == 0){

                if(pause2 == 0){

                    if( first_loop == 1){
                        pontos();
                        displayBoard();
                        drawPaddles();
                        first_loop = 0;
                    }

                    ADXL345_XYZ_Read(XYZ);
                    
                    handleMouse(mouseFile);

                    // Checa se a placa foi inclinada para baixo e move a peça
                    if(XYZ[1]*mg_per_lsb <= -200 && paddle1Y < HEIGHT - paddleSize / 2 ){
                        paddle1Y++;
                        drawPaddles();
                    }// Checa se a placa foi inclinada para cima e move a peça
                    else if(XYZ[1]*mg_per_lsb >= 200 && paddle1Y > paddleSize / 2){
                        paddle1Y--;
                        drawPaddles();
                    }

                    updateBall();

                    // Checa se o botao 0 foi pressionado e caso seja, pausa o jogo
                    if (button_value == 14 && button == 15) {
                        button_value = 15;
                        printf("Pausando...\n");
                        pause2 = 1;
                    }

                    if(score1 == 7|| score2 == 7){
                        estado = 2;
                    }
                    
                }else{

                    if(first_loop == 0){
                        displayBoard();
                        pontos();
                        drawPaddles();
                        draw_ball();
                        jogo_pause();
                    }
                    // Despausa o jogo
                    if (button_value == 14 && button == 15 && opt == 0) {
                        button_value = 15;
                        printf("Continuando...\n");
                        pause2 = 0;
                        first_loop = 0;
                    // Sai da partida 
                    } else if(button_value == 14 && button == 15 && opt == 1){
                        button_value = 15;
                        estado = 1;
                        opt = 0;
                        pause2 = 0;
                        score1 = 0;
                        score2 = 0;
                        printf("saindo...\n");
                        initBoard();
                        limpar_tudo();

                    }

                    first_loop = 1;
                }
            }else if(estado == 2){
                if(score1 == 7){
                    draw_sprite(1, 1, 65, 345, 24); //P1 ganhou
                    draw_sprite(1, 1, 65, 355, 24);
                }else if(score2 == 7){
                    draw_sprite(2, 1, 537, 345, 24); //P2 ganhou
                    draw_sprite(2, 1, 537, 355, 24);
                }

                // Desenha botão de voltar
                draw_horizontal_text(4310, 0b000000111, 3);
                draw_vertical_text(4393, 0b000000111, 2);
                draw_horizontal_text(4546, 0b000000111, 7);
                draw_text(4467, 0b000000111);
                draw_text(4627, 0b000000111);
                draw_text(4388, 0b000000111);
                draw_text(4708, 0b000000111);

                if(button_value == 14 && button == 15 && opt == 0){
                        button_value = 15;
                        estado = 1;
                        opt = 0;
                        pause2 = 0;
                        score1 = 0;
                        score2 = 0;
                        printf("saindo...\n");
                        initBoard();
                        limpar_tudo();
                    }

                }

            if(estado == 1 || pause2 == 1){
                // Muda a opção a ser pressionada
                if (button_value == 13 && button == 15) {
                    button_value = 15;
                    opt = !opt;
                    
                    first_loop = 0;
                } 
            }

            // Inicia o jogo
            if (button_value == 14 && button == 15 && opt == 0) {
                button_value = 15;
                printf("iniciando...\n");
                estado = 0;
                limpar_tudo();
            // Sai do jogo 
            } else if(button_value == 14 && button == 15 && opt == 1){
                button_value = 15;
                printf("saindo...\n");
                limpar_tudo();
                exit(0);
            }
        }   

     } else {
        printf("Incorrect device ID\n");
    }
    return 0;
}
