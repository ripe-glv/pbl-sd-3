#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>

// Endereços usados para mapeamento e funcionalidades dos botões
// #define LW_BRIDGE_BASE 0xFF200000
// #define LW_BRIDGE_SPAN 0x00005000
// #define KEY_BASE 0x00000050
// #define BUTTON_0_MASK 0x01
// #define BUTTON_1_MASK 0x02
// #define BUTTON_2_MASK 0x04
// #define BUTTON_3_MASK 0x08

// Tamanho do tabuleiro do jogo
#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20
#define FALL_DELAY 500000 // 1 segundo

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

extern void mapm();
extern void draw_box(int posicao, int cor);
extern void draw_text(int posicao, int cor);
extern void limpar_tela();
extern int button_pressed();

// Tabuleiro do jogo
int board[BOARD_HEIGHT][BOARD_WIDTH];

// A variavel shapes armazena todos os blocos possiveis de jogo e suas variacoes, vertical, horizontal, invertido verticalmente e invertido horizontalmente
int shapes[7][4][4][4] = {
 // I
 {
 { {0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0} },
 { {0,1,0,0}, {0,1,0,0}, {0,1,0,0}, {0,1,0,0} },
 { {0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0} },
 { {0,1,0,0}, {0,1,0,0}, {0,1,0,0}, {0,1,0,0} }
 },
 // O
 {
 { {1,1}, {1,1} },
 { {1,1}, {1,1} },
 { {1,1}, {1,1} },
 { {1,1}, {1,1} }
 },
 // T
 {
 { {0,1,0}, {1,1,1}, {0,0,0} },
 { {0,1,0}, {0,1,1}, {0,1,0} },
 { {0,0,0}, {1,1,1}, {0,1,0} },
 { {0,1,0}, {1,1,0}, {0,1,0} }
 },
 // L
 {
 { {0,0,1}, {1,1,1}, {0,0,0} },
 { {0,1,0}, {0,1,0}, {0,1,1} },
 { {0,0,0}, {1,1,1}, {1,0,0} },
 { {1,1,0}, {0,1,0}, {0,1,0} }
 },
 // J
 {
 { {1,0,0}, {1,1,1}, {0,0,0} },
 { {0,1,1}, {0,1,0}, {0,1,0} },
 { {0,0,0}, {1,1,1}, {0,0,1} },
 { {0,1,0}, {0,1,0}, {1,1,0} }
 },
 // Z
 {
 { {1,1,0}, {0,1,1}, {0,0,0} },
 { {0,0,1}, {0,1,1}, {0,1,0} },
 { {1,1,0}, {0,1,1}, {0,0,0} },
 { {0,0,1}, {0,1,1}, {0,1,0} }
 },
 // S
 {
 { {0,1,1}, {1,1,0}, {0,0,0} },
 { {0,1,0}, {0,1,1}, {0,0,1} },
 { {0,1,1}, {1,1,0}, {0,0,0} },
 { {0,1,0}, {0,1,1}, {0,0,1} }
 }
};

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
// unsigned int * sysmgr_virtual_ptr;

// Variaveis de controle e variaveis de execucao do jogo
int currentShape;
int currentRotation;
int currentX;
int currentY;
int menu = 1;
int scr = 0;
int hscr[3] = {0, 0, 0};
int pause2 = 0;
int opt = 0;
int jogando = 0;

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


// Cria o tabuleiro do jogo
void initBoard() {
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        for (int j = 0; j < BOARD_WIDTH; j++) {
            board[i][j] = 0;
        }
    }
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

int* ler_pontos(int pontos, int* tamanho_array) {
    // Se o número for negativo, converte para positivo
    if (pontos < 0) {
        pontos = -pontos;
    }

    // Calcula o número de dígitos
    int temp = pontos;
    int digitos = 0;
    while (temp > 0) {
        temp /= 10;
        digitos++;
    }

    // Se o número for zero, então há 1 dígito (o próprio zero)
    if (pontos == 0) {
        digitos = 1;
    }

    // Aloca o vetor para armazenar os dígitos
    int* digitos_array = (int*)malloc(digitos * sizeof(int));

    // Preenche o vetor com os dígitos
    for (int i = 0; i < digitos; i++) {
        digitos_array[i] = pontos % 10;  // Extrai o dígito da unidade
        pontos /= 10;  // Remove o último dígito
    }

    // Reverte o vetor para ter a ordem correta (do primeiro para o último dígito)
    for (int i = 0; i < digitos / 2; i++) {
        int temp = digitos_array[i];
        digitos_array[i] = digitos_array[digitos - i - 1];
        digitos_array[digitos - i - 1] = temp;
    }

    *tamanho_array = digitos;  // Define o tamanho do array
    return digitos_array;  // Retorna o vetor de dígitos
}


// Função que recebe posicao e o numero especificado e o Desenha
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
            draw_text(posicao + 80, 0b111111111);
            draw_text(posicao + 1, 0b111111111);
            draw_text(posicao + 82, 0b111111111);
            draw_text(posicao + 161, 0b111111111);
            draw_text(posicao + 240, 0b111111111);
            draw_horizontal_text(posicao + 320, 0b111111111, 3);
            break;

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
            draw_horizontal_text(posicao, 0b111111111, 2);
            draw_vertical_text(posicao - 1, 0b111111111, 5);
            draw_horizontal_text(posicao + 320, 0b111111111, 2);
            draw_vertical_text(posicao + 161, 0b111111111, 2);
            draw_text(posicao + 160, 0b111111111);
            break;

        case 7:

            // Desenha o 7 inclinado 
            draw_horizontal_text(posicao, 0b111111111, 3);
            draw_vertical_text(posicao + 82, 0b111111111, 4);
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
// Função que Desenha todas as letras de "Tetris" com sombra e cores diferentes
void draw_menu() {
    // Desenha a letra T
    draw_horizontal(565, 0b110110110, 5);
    draw_vertical(729, 0b110110110, 4);

    // Desenha a letra E (Verde)
    draw_vertical(577, 0b001101010, 5);
    draw_horizontal(579, 0b001101010, 4);
    draw_horizontal(899, 0b001101010, 3);
    draw_horizontal(1219, 0b001101010, 4);

    // Desenha a letra T (segunda vez - Azul)
    draw_horizontal(589, 0b101011001, 5);
    draw_vertical(753, 0b101011001, 4);

    // Desenha a letra R (Amarelo)
    draw_vertical(601, 0b001110111, 5);
    draw_horizontal(603, 0b001110111, 2);
    draw_box(767, 0b001110111);
    draw_horizontal(923, 0b001110111, 2);
    draw_box(1087, 0b001110111);
    draw_box(1247, 0b001110111);

    // Desenha a letra I
    draw_horizontal(611, 0b101101001, 5);
    draw_vertical(695, 0b101101001, 4);
    draw_horizontal(1251, 0b101101001, 5);

    // Desenha a letra S
    draw_horizontal(625, 0b001010111, 4);
    draw_box(783, 0b001010111);
    draw_horizontal(945, 0b001010111, 3);
    draw_box(1111, 0b001010111);
    draw_horizontal(1263, 0b001010111, 4);
}

// Função principal que exibe na tela as paginas do jogo
void drawBoard(){

    // A variavel menu faz o controle para checar se o usuario esta no menu do jogo ou se esta jogando
    if( menu == 1 ){
        draw_menu();

        scr = 0;
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

    }else{

        // Desenhando as bordas do jogo
        draw_vertical(748, 0b111111111, 20);
        draw_horizontal(3948, 0b111111111, 12);
        draw_vertical(770, 0b111111111, 20);

        int tamanho_array_hscr1;
        int* digitos_hscr1 = ler_pontos(hscr[0], &tamanho_array_hscr1);  // Lê os pontos e define o tamanho do array

        int tamanho_array_hscr2;
        int* digitos_hscr2 = ler_pontos(hscr[1], &tamanho_array_hscr2);  // Lê os pontos e define o tamanho do array

        int tamanho_array_hscr3;
        int* digitos_hscr3 = ler_pontos(hscr[2], &tamanho_array_hscr3);  // Lê os pontos e define o tamanho do array

        // Desenha a letra H
        draw_vertical_text(728, 0b111111111, 5);
        draw_text(889, 0b111111111);
        draw_vertical_text(730, 0b111111111, 5);
        // Desenha a letra S
        draw_horizontal_text(733, 0b111111111, 2);
        draw_text(812, 0b111111111);
        draw_horizontal_text(892, 0b111111111, 3);
        draw_text(974, 0b111111111);
        draw_horizontal_text(1052, 0b111111111, 2);

        // Desenha o ponto
        draw_text(1056, 0b111111111);

        // Desenha o 1
        draw_vertical_text(1283, 0b111111111, 5);
        draw_text(1362  , 0b111111111);

        // Hífen
        draw_horizontal_text(1446, 0b111111111, 2);

        // Desenha o highscore 1
        int pos_hscr1 = 1290;
        for (int i = 0; i < tamanho_array_hscr1; i++) {
            Desenha_number(pos_hscr1, digitos_hscr1[i]);
            pos_hscr1 += 4;
        }

        // Liberar a memória alocada para o array
        free(digitos_hscr1);

        // Desenha o 2
        draw_text(1842, 0b111111111);
        draw_text(1763, 0b111111111);
        draw_text(1844, 0b111111111);
        draw_text(1923, 0b111111111);
        draw_text(2002, 0b111111111);
        draw_horizontal_text(2082, 0b111111111, 3);
        
        // Hífen
        draw_horizontal_text(1926, 0b111111111, 2);


        // Desenha o highscore 2
        int pos_hscr2 = 1770;
        for (int i = 0; i < tamanho_array_hscr2; i++) {
            Desenha_number(pos_hscr2, digitos_hscr2[i]);
            pos_hscr2 += 4;
        }

        // Liberar a memória alocada para o array
        free(digitos_hscr2);


        // Deseha o 3
        draw_text(2242, 0b111111111);
        draw_text(2243, 0b111111111);
        draw_text(2324, 0b111111111);
        draw_text(2403, 0b111111111);
        draw_text(2562, 0b111111111);
        draw_text(2563, 0b111111111);
        draw_text(2484, 0b111111111);

        // Hífen
        draw_horizontal_text(2406, 0b111111111, 2);

        // Desenha o highscore 3
        int pos_hscr3 = 2250;
        for (int i = 0; i < tamanho_array_hscr3; i++) {
            Desenha_number(pos_hscr3, digitos_hscr3[i]);
            pos_hscr3 += 4;
        }

        // Liberar a memória alocada para o array
        free(digitos_hscr3);

        // Desenha o P
        draw_vertical_text(775, 0b111111111, 5);
        draw_text(776, 0b111111111);
        draw_text(857, 0b111111111);
        draw_text(936, 0b111111111);

        // Desenha o T
        draw_horizontal_text(779, 0b111111111, 3);
        draw_vertical_text(860, 0b111111111, 4);

        // Desenha o S
        draw_horizontal_text(784, 0b111111111, 2);
        draw_text(863, 0b111111111);
        draw_horizontal_text(943, 0b111111111, 3);
        draw_text(1025, 0b111111111);
        draw_horizontal_text(1103, 0b111111111, 2);

        // Desenha a quantidade atual de pontos

        int tamanho_array_pts;
        int* digitos_scr = ler_pontos(scr, &tamanho_array_pts);  // Lê os pontos e define o tamanho do array

        int pos_pts = 1336;
        for (int i = 0; i < tamanho_array_pts; i++) {
            Desenha_number(pos_pts, digitos_scr[i]);
            pos_pts += 4;
        }

        // Liberar a memória alocada para o array
        free(digitos_scr);

        
        // Loop for que Desenha a matriz do jogo (peças ja caidas e espaços vazios)
        for (int i = 0; i < BOARD_HEIGHT; i++) {
            for (int j = 0; j < BOARD_WIDTH; j++) {
                if (board[i][j] == 1) {
                    draw_box(750+(j*2)+(i*160), 0b101101101);
                } else {
                    draw_box(750+(j*2)+(i*160), 0b000000000);
                } 
            } 
        }


        // Desenha a peça atual
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                if (shapes[currentShape][currentRotation][i][j] == 1) {
                    draw_box(750+(currentX + j)*2 + (currentY + i) * 160, 0b000111000);
                }
            }
        }

        // If que checa se o jogo esta em pause
        if(pause2 == 1){
            // If que checa a opção a ser inserida no menu de pausa e muda o indicador (>)
            if(opt == 0){
                draw_vertical(750, 0b000000000, 20);
                draw_vertical(752, 0b000000000, 20);
                draw_vertical(754, 0b000000000, 20);
                draw_vertical(756, 0b000000000, 20);
                draw_vertical(758, 0b000000000, 20);
                draw_vertical(760, 0b000000000, 20);
                draw_vertical(762, 0b000000000, 20);
                draw_vertical(764, 0b000000000, 20);
                draw_vertical(766, 0b000000000, 20);
                draw_vertical(768, 0b000000000, 20);

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
                draw_vertical(750, 0b000000000, 20);
                draw_vertical(752, 0b000000000, 20);
                draw_vertical(754, 0b000000000, 20);
                draw_vertical(756, 0b000000000, 20);
                draw_vertical(758, 0b000000000, 20);
                draw_vertical(760, 0b000000000, 20);
                draw_vertical(762, 0b000000000, 20);
                draw_vertical(764, 0b000000000, 20);
                draw_vertical(766, 0b000000000, 20);
                draw_vertical(768, 0b000000000, 20);

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

        // Jogando = 2 significa que o jogador perdeu
        if(jogando == 2){
            // Escrever Game Over
            // Desenha a letra G
            draw_horizontal_text(2352, 0b000000111, 3);
            draw_vertical_text(2432, 0b000000111, 3);
            draw_horizontal_text(2672, 0b000000111, 3);
            draw_text(2594, 0b000000111);

            // Desenha a letra A
            draw_horizontal_text(2356, 0b000000111, 3);
            draw_vertical_text(2436, 0b000000111, 4);
            draw_vertical_text(2438, 0b000000111, 4);
            draw_text(2517, 0b000000111);

            // Desenha a letra M
            draw_vertical_text(2360, 0b000000111, 5);
            draw_text(2441, 0b000000111);
            draw_vertical_text(2362, 0b000000111, 5);

            // Desenha a letra E
            draw_vertical_text(2364, 0b000000111, 5);
            draw_horizontal_text(2365, 0b000000111, 2);
            draw_horizontal_text(2525, 0b000000111, 2);
            draw_horizontal_text(2685, 0b000000111, 2);

            // Desenha a letra O
            draw_horizontal_text(2832, 0b000000111, 3);
            draw_vertical_text(2912, 0b000000111, 3);
            draw_vertical_text(2914, 0b000000111, 3);
            draw_horizontal_text(3152, 0b000000111, 3);

            // Desenha a letra V
            draw_vertical_text(2836, 0b000000111, 4);
            draw_text(3157, 0b000000111);
            draw_vertical_text(2838, 0b000000111, 4);

            // Desenha a letra E
            draw_vertical_text(2840, 0b000000111, 5);
            draw_horizontal_text(2841, 0b000000111, 2);
            draw_horizontal_text(3001, 0b000000111, 2);
            draw_horizontal_text(3161, 0b000000111, 2);

            // Desenha a letra R
            draw_horizontal_text(2844, 0b000000111, 2);
            draw_vertical_text(2924, 0b000000111, 4);
            draw_vertical_text(2926, 0b000000111, 1);
            draw_vertical_text(3005, 0b000000111, 1);
            draw_vertical_text(3086, 0b000000111, 2);

            // Desenha botão de voltar
            draw_horizontal_text(4310, 0b000000111, 3);
            draw_vertical_text(4393, 0b000000111, 2);
            draw_horizontal_text(4546, 0b000000111, 7);
            draw_text(4467, 0b000000111);
            draw_text(4627, 0b000000111);
            draw_text(4388, 0b000000111);
            draw_text(4708, 0b000000111);
        } 
    }

}

// Função que remove a linha caso totalmente preenchida e move as que estão acima dela uma unidade abaixo, ao final soma 100 ao score do jogdor
void removeLine(int y) {

    for (int j = 0; j < BOARD_WIDTH; j++) {
        board[y][j] = 0;
    }

    for (int i = y; i > 0; i--) {
    for (int j = 0; j < BOARD_WIDTH; j++) {
        board[i][j] = board[i - 1][j];
        } 
    }

    for (int j = 0; j < BOARD_WIDTH; j++) {
        board[0][j] = 0;
    }

    scr += 10;
    limpar_tela();
}

// Checa se existem linhas preenchidas na matriz e as remove
void checkLines() {

    for (int i = 0; i < BOARD_HEIGHT; i++) {
        int fullLine = 1;
        for (int j = 0; j < BOARD_WIDTH; j++) {
            if (board[i][j] == 0) {
                fullLine = 0;
                break;
            }
        }
    if (fullLine) {
        removeLine(i);
    }
    }
}

// Função que "Encaixa" a peça na matriz do jogo, alterando o valor anterior de 0 (vazio) para 1 (bloco)
void placeShape() {

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (shapes[currentShape][currentRotation][i][j] == 1) {
                board[currentY + i][currentX + j] = 1;
            }
        }
    }
    checkLines();
}

// Checa se houve colisao da peça atual com a matriz do jogo
int checkCollision(int x, int y, int rotation) {

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (shapes[currentShape][rotation][i][j] == 1) {
                int boardX = x + j;
                int boardY = y + i;
                if (boardX < 0 || boardX >= BOARD_WIDTH || boardY >= BOARD_HEIGHT || (boardY >= 0 && board[boardY][boardX] == 1)) {
                    return 1;
                }
            }
        }
    }

    return 0;
}

// Gera uma nova peça
void newShape() {
    currentShape = rand() % 7;
    currentRotation = 0;
    currentX = BOARD_WIDTH / 2 - 2;
    currentY = 0;
    if (checkCollision(currentX, currentY, currentRotation)) {
        printf("Game Over!\n");
        // Jogando = 2 simboliza game over
        jogando = 2;
    }
}

// Função que faz o movimento para baixo da peça
void moveDown() {
    if (!checkCollision(currentX, currentY + 1, currentRotation)) {
        currentY++;
    } else {
        placeShape();
        newShape();
    }
}

// Função para rotacionar a peça
void rotateShape() {
    int newRotation = (currentRotation + 1) % 4;
    if (!checkCollision(currentX, currentY, newRotation)) {
        currentRotation = newRotation;
    }
}

// Função para movar a peça para a esquerda
void moveLeft() {
    if (!checkCollision(currentX - 1, currentY, currentRotation)) {
        currentX--;
    }
}

// Função para movar a peça para a direita
void moveRight() {
    if (!checkCollision(currentX + 1, currentY, currentRotation)) {
        currentX++;
    }
}

int main() {
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

        time_t lastFallTime = time(NULL);

        // Mapeia a memória
        mapm();
        limpar_tela();
        
        int button_value = 0;
        int hs_controller = 0;
        while (1) {
            int button = button_pressed();
            if (button != 15) {
                button_value = button;
            }

            // If checa se o jogador esta no jogo (menu == 0) e se não perdeu (jogando != 2)
            if(menu == 0 && jogando != 2){
                jogando = 1;
            // If checa se o jogo não esta pausado (pause2 == 0) e se esta em jogo (jogando == 1)
            if(pause2 == 0 && jogando == 1){

                ADXL345_XYZ_Read(XYZ);
                // Checa se a placa foi inclinada a direita e move a peça
                if(XYZ[0]*mg_per_lsb >= 150){
                    moveRight();
                    drawBoard();
                    usleep(50000); 
                // Checa se a placa foi inclinada a esquerda e move a peça
                }else if(XYZ[0]*mg_per_lsb <= -150){
                    moveLeft();
                    drawBoard();
                    usleep(50000); 
                }
                // Checa se a placa foi inclinada para baixo e move a peça
                if(XYZ[1]*mg_per_lsb <= -200){
                    moveDown();
                    drawBoard();
                }

                // Checa se o botao 0 foi pressionado e caso seja, pausa o jogo
                if (button_value == 14 && button == 15) {
                    button_value = 15;
                    printf("Pausando...\n");
                    pause2 = 1;
                }
                // Checa se o botao 1 foi pressionado e caso seja, gira a peça atual
                if (button_value == 13 && button == 15) {
                    button_value = 15;
                    rotateShape();
                    drawBoard();
                }

                // Checagem para queda automatica da peça
                if (difftime(time(NULL), lastFallTime) >= FALL_DELAY / 500000.0) {
                    moveDown();
                    drawBoard();
                    lastFallTime = time(NULL);
                }

                // Desenha a matriz na tela do monitor
                usleep(50000); 

            // Checa se o jogo foi pausado 
            } else if(pause2 == 1 && jogando == 1){
                if (hs_controller == 0){
                    drawBoard();
                    hs_controller = 1;
                }
                

            // Muda a opção a ser pressionada
            if (button_value == 13 && button == 15) {
                button_value = 15;
                opt = !opt;
                limpar_tela();
                hs_controller = 0;
            } 

            // Despausa o jogo
            if (button_value == 14 && button == 15 && opt == 0) {
                button_value = 15;
                printf("Continuando...\n");
                pause2 = 0;
                limpar_tela();
                hs_controller = 0;

            // Sai da partida 
            } else if(button_value == 14 && button == 15 && opt == 1){
                button_value = 15;
                menu = 1;
                jogando = 0;
                opt = 0;
                pause2 = 0;
                scr = 0;
                printf("saindo...\n");
                limpar_tela();
                hs_controller = 0;
            }

            }

            // Checagem para reiniciar o tabuleiro sempre que o jogador vai do menu inicial para o jogo
            }else if(jogando == 0){

                initBoard();
                newShape();

                drawBoard();

            if (button_value == 13 && button == 15) {
                button_value = 15;
                opt = !opt;
                limpar_tela();
            } 

            // Inicia o jogo
            if (button_value == 14 && button == 15 && opt == 0) {
                button_value = 15;
                printf("iniciando...\n");
                menu = 0;
                limpar_tela();
            // Sai do jogo 
            } else if(button_value == 14 && button == 15 && opt == 1){
                button_value = 15;
                printf("saindo...\n");
                limpar_tela();
                exit(0);
            }

            // Checa se o jogador perdeu (jogando == 2) e caso o score dele seja maior que um dos 3 recordes, salva o score dele
            }else if(jogando == 2){

                if(scr > hscr[0]){
                    hscr[2] = hscr[1];
                    hscr[1] = hscr[0];
                    hscr[0] = scr;
                    if (hs_controller == 0){
                        limpar_tela();
                        drawBoard();
                        hs_controller = 1;
                    }
                    scr = 0;
                }else if(scr > hscr[1]){
                    hscr[2] = hscr[1];
                    hscr[1] = scr;
                    if (hs_controller == 0){
                        limpar_tela();
                        drawBoard();
                        hs_controller = 1;
                    }
                    scr = 0;
                }else if(scr > hscr[2]){
                    hscr[2] = scr;
                    if (hs_controller == 0){
                        limpar_tela();
                        drawBoard();
                        hs_controller = 1;
                    }
                    scr = 0;
                }

                // Volta ao menu
                if (button_value == 14 && button == 15 && opt == 0) {
                    button_value = 15;
                    menu = 1;
                    jogando = 0;
                    hs_controller = 0;
                    opt = 0;
                    pause2 = 0;
                    scr = 0;
                    printf("saindo...\n");
                    limpar_tela();
                }

            }
        }

    } else {
        printf("Incorrect device ID\n");
    }
 
 return 0;
}