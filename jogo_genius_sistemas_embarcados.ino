/*
	LiquidCrystal_I2C: 
    Biblioteca para controlar displays LCD via protocolo I2C
*/ 
#include <LiquidCrystal_I2C.h>

// Inicialização do objeto LCD com comunicação I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ========== CONFIGURAÇÃO DE HARDWARE ==========
// Pinos dos LEDs (todos no Port D)
#define LED_VERMELHO  PD2 // Pino 2 (bit 2 do PORTD)
#define LED_AMARELO   PD3 // Pino 3 (bit 3 do PORTD)
#define LED_AZUL      PD4 // Pino 4 (bit 4 do PORTD)
#define LED_VERDE     PD5 // Pino 5 (bit 5 do PORTD)

// Pino do Buzzer (Port D)
#define BUZZER        PD7 // Pino 7 (bit 7 do PORTD)

// Pinos dos Botões (todos no Port B)
#define BTN_VERMELHO  PB0  // Pino 8  (bit 0 do PORTB)
#define BTN_AMARELO   PB1  // Pino 9  (bit 1 do PORTB)
#define BTN_AZUL      PB2  // Pino 10 (bit 2 do PORTB)
#define BTN_VERDE     PB3  // Pino 11 (bit 3 do PORTB)

// Botão para iniciar o jogo (Port D)
// Quando pressionado, ativa o modo de jogo
#define BTN_INICIAR   PD6 // Pino 6 (bit 6 do PORTD)

// Botão para acessar o menu de dificuldade (Port B)
// Permite ao jogador escolher entre Fácil, Médio e Difícil
// A cada pressionamento, cicla entre os 3 níveis de dificuldade
#define BTN_MENU     PB4 // Pino 12 (bit 4 do PORTB)

// ========== ESTRUTURAS DE DADOS ==========
/*
  Array que armazena a sequência do jogo (máximo 12 rodadas)
  Cada posição guarda um número de 0 a 3 que representa:
    0 -> LED Vermelho
    1 -> LED Amarelo
    2 -> LED Azul
    3 -> LED Verde
  Exemplo: {0, 2, 1, 2, ...} = Vermelho -> Azul -> Amarelo -> Azul -> ...
*/
uint8_t sequencia[12] = {};

// Array com os pinos dos botões na mesma ordem dos LEDs
// Facilita o acesso aos botões através de índices
// Marcado como const para economizar RAM
const uint8_t botoes[4] = {BTN_VERMELHO, BTN_AMARELO, BTN_AZUL, BTN_VERDE};

// Array com os pinos dos LEDs na mesma ordem
// Permite acender qualquer LED usando: leds[numero]
// Marcado como const para economizar RAM
const uint8_t leds[4] = {LED_VERMELHO, LED_AMARELO, LED_AZUL, LED_VERDE};

// Array com as frequências de som para cada LED
// Cada LED toca uma nota musical diferente: LÁ3, DÓ5, SOL5 e MI6
// Marcado como const para economizar RAM
const int tons[4] = {220, 523, 784, 1319};

// ========== VARIÁVEIS DE CONTROLE DO JOGO ==========
/*
  Contador de rodadas:
  - indica quantos passos já foram adicionados à sequência
  - Quando chega a 12, o jogador venceu o jogo
*/
uint8_t rodada = 0;

/*
  Contador de passos:
  - usado para verificar em qual posição da sequência o jogador está
  - Incrementado a cada acerto do jogador
*/
uint8_t passo = 0;

// Armazena qual botão foi pressionado pelo jogador
uint8_t botaoPressionado = 0;


uint8_t opcaoMenu = 0;

// Flag que indica se o jogador errou a sequência
bool perdeuJogo = false;

/*
  Flag que controla o estado do jogo:
  - false: Jogo em modo de espera (aguardando pressionar o botão iniciar)
  - true: Jogo ativo (rodadas em andamento)
  
  Permite que o Arduino fique em modo de espera sem consumir recursos
  até que o jogador pressione o botão de iniciar
*/
bool jogoAtivo = false;

bool menuJogoAtivo = true;

bool SonsLeds = false;

bool telaAtualizada = false;

uint8_t estadoAnterior = 255; // 0: Menu, 1: Dificuldade, 2: Jogo, 3: Sons

// ========== SISTEMA DE DIFICULDADE ==========
/*
  Sistema de dificuldade dinâmica com 3 níveis:
  
  Os valores de velocidade são aplicados pela função aplicarDificuldade()
  conforme o nível selecionado pelo jogador:
  
  FÁCIL (nivelDificuldade = 0):
  - velocidade1: 1500ms (1.5s entre rodadas - mais tempo para pensar)
  - velocidade2: 400ms (LEDs ficam acesos mais tempo - mais fácil de ver)
  - velocidade3: 250ms (mais intervalo entre LEDs - sequência mais clara)
  
  MÉDIO (nivelDificuldade = 1):
  - velocidade1: 1000ms (1s entre rodadas - tempo padrão)
  - velocidade2: 300ms (LEDs acesos tempo padrão)
  - velocidade3: 200ms (intervalo padrão entre LEDs)
  
  DIFÍCIL (nivelDificuldade = 2):
  - velocidade1: 700ms (0.7s entre rodadas - pouco tempo para pensar)
  - velocidade2: 200ms (LEDs acesos rapidamente - difícil de acompanhar)
  - velocidade3: 150ms (pouco intervalo - sequência muito rápida)
  
  Quanto menores os valores, mais rápido e difícil fica o jogo
  
  PROGRESSÃO ADICIONAL:
  Na rodada 7, as velocidades são reduzidas em 20% automaticamente,
  aumentando ainda mais a dificuldade independente do nível inicial
*/
int velocidade1 = 0; // Delay entre rodadas (será definido por aplicarDificuldade())
int velocidade2 = 0; // LED aceso na sequência (será definido por aplicarDificuldade())
int velocidade3 = 0; // Intervalo entre LEDs (será definido por aplicarDificuldade())

/*
  Armazena o nível de dificuldade selecionado pelo jogador
  
  Valores possíveis:
  - 0 = FÁCIL (velocidades mais lentas, mais tempo para pensar)
  - 1 = MÉDIO (velocidades padrão, equilíbrio entre desafio e jogabilidade)
  - 2 = DIFÍCIL (velocidades rápidas, requer reflexos aguçados)
  
  O valor é alterado no menu de dificuldade através do botão BTN_MENU
  e aplicado pela função aplicarDificuldade() antes do jogo iniciar
*/
uint8_t nivelDificuldade = 0; 


/*
  Flag que controla se o menu de dificuldade está ativo
  
  Estados:
  - true: Menu de dificuldade visível no LCD
           Jogador pode navegar com BTN_MENU e confirmar com BTN_INICIAR
           Função menuDificuldade() está em execução
  - false: Menu fechado, jogo em andamento
*/
bool menuDificuldadeAtivo = false;

// Máscara global dos leds para facilitar ligar/desligar todos os LEDs ao mesmo tempo
const uint8_t ledsMascara = (1 << LED_VERMELHO) | (1 << LED_AMARELO) | (1 << LED_AZUL) | (1 << LED_VERDE);

void setup()
{ 
  /*
    Sequência de inicialização do display LCD:
    1. init() - Inicializa a comunicação I2C com o display
    2. backlight() - Liga a luz de fundo do display para visualização
  */
  lcd.init();
  lcd.backlight(); 

  // Configura LEDs (PD2, PD3, PD4, PD5) como saída
  // Combina múltiplos bits usando OR (|) para configurar todos de uma vez
  DDRD |= (1 << LED_VERMELHO) | (1 << LED_AMARELO) | (1 << LED_AZUL) | (1 << LED_VERDE);
  
  // Configura o Buzzer (PD7) como saída
  DDRD |= (1 << BUZZER);
  
  // Configura botão iniciar (PD6) e botão menu (PB4) como entrada
  // &= ~(NOT) zera o bit, configurando como entrada
  DDRD &= ~(1 << BTN_INICIAR);
  DDRB &= ~(1 << BTN_MENU);

  // Habilita resistores de pull-up internos para os botões iniciar e menu
  PORTD |= (1 << BTN_INICIAR); 
  PORTB |= (1 << BTN_MENU);

  // Configura botões de jogo (PB0, PB1, PB2, PB3) como entrada
  DDRB &= ~( (1 << BTN_VERMELHO) | (1 << BTN_AMARELO) | (1 << BTN_AZUL) | (1 << BTN_VERDE));

  // Habilita resistores de pull-up internos para os botões de jogo
  PORTB |= ( (1 << BTN_VERMELHO) | (1 << BTN_AMARELO) | (1 << BTN_AZUL) | (1 << BTN_VERDE)); 
  
  // Inicializa o gerador de números aleatórios usando ruído da porta analógica
  // Isso garante que cada jogo terá uma sequência diferente
  randomSeed(analogRead(A0));
}

void loop()
{
  /*
    O jogo opera em dois estados principais:
    
    ESTADO 1 - MENU DE DIFICULDADE (jogoAtivo = false):
    - Exibe opções de dificuldade no LCD
    - Aguarda seleção do jogador via BTN_MENU
    - Aguarda confirmação via BTN_INICIAR
    
    ESTADO 2 - JOGO ATIVO (jogoAtivo = true):
    - Executa a lógica principal do Genius
    - Gerencia rodadas, sequências e validações
    - Detecta vitória ou derrota
    
    Transições:
    - Menu → Jogo: quando BTN_INICIAR é pressionado no menu
    - Jogo → Menu: quando jogo termina (vitória ou derrota)
  */
  if (jogoAtivo) {
    atualizarTela(2);
    jogoGenius();
  } 
  else if (menuDificuldadeAtivo) {
    atualizarTela(1);
    menuDificuldade();
  } 
  else if (SonsLeds) {
    atualizarTela(3);
    ouvirLeds();
  } 
  else {
    atualizarTela(0);
    menuJogo();
  }
}


// ========== FUNÇÕES DO JOGO ==========

void atualizarTela(uint8_t estadoAtual) {
  if (estadoAtual != estadoAnterior) {
    telaAtualizada = false; 
    estadoAnterior = estadoAtual;
  }
}

/*
  Lógica principal do Jogo Genius.
  Gerencia todo o fluxo de uma partida do jogo:
  - Adiciona novos passos à sequência
  - Reproduz a sequência para o jogador
  - Aguarda e valida as jogadas do jogador
  - Controla progressão de dificuldade
  - Detecta vitória ou derrota
  
  Esta função só é executada quando jogoAtivo == true
*/
void  jogoGenius(){

  // Verifica se o jogador perdeu o jogo ou completou todas as rodadas
  if(perdeuJogo == true){

    // Se perdeu ou venceu, reseta tudo e volta ao modo de espera
    limparJogo();

    // Retorna imediatamente para evitar que o código continue executando
    // após limpar o jogo
    return;
  }

  // Fluxo normal do jogo:
  proximaRodada();       // 1. Adiciona um novo passo à sequência
  visorRodadas();        // 2. Atualiza o LCD com a rodada atual
  reproduzirSequencia(); // 3. Mostra a sequência completa ao jogador
  esperarJogador();      // 4. Aguarda o jogador repetir a sequência

  // Verifica se o jogador errou durante esperarJogador()
  // Se errou, retorna imediatamente sem executar o resto do código
  if(perdeuJogo) return;

  // Delay entre rodadas
  delay(velocidade1);

  /*
    Na rodada 7, aumenta automaticamente a dificuldade reduzindo
    os tempos de espera em 20%, independente do nível inicial selecionado.
  */
  if(rodada == 7){
    velocidade1 -= (velocidade1 * 20) / 100; // Reduz 20% do valor atual
    velocidade2 -= (velocidade2 * 20) / 100; // Reduz 20% do valor atual
    velocidade3 -= (velocidade3 * 20) / 100; // Reduz 20% do valor atual
  }

  // Se o jogador completou todas as 12 rodadas, ele venceu!
  // Exibe mensagem de vitória e toca melodia especial
  if(rodada == 12){
    visorVitoria();    // Exibe mensagem de vitória no LCD
    venceuJogo();       // Toca melodia de vitória com show de luzes
    perdeuJogo = true;  // Usa a mesma flag de derrota para resetar o jogo
  }
  
}

/*
  Adiciona um novo passo aleatório à sequência
  - Sorteia um número de 0 a 3 (representando as 4 cores)
  - Adiciona na posição atual da rodada
  - Incrementa o contador de rodadas
*/
void proximaRodada(){
  sequencia[rodada] = random(4);
  rodada ++;
}

/*
  Reproduz a sequência completa para o jogador
  - Percorre todos os passos da sequência atual
  - Para cada passo: acende o LED correspondente e toca seu som
  - Utiliza as variáveis de velocidade definidas pela dificuldade selecionada
*/
void reproduzirSequencia(){
  
  // Loop que percorre toda a sequência acumulada até agora
  for(uint8_t i = 0; i < rodada; i++){
    
    // Toca o som correspondente ao LED atual
    tone(BUZZER, tons[sequencia[i]], 250);
    PORTD |= (1 << leds[sequencia[i]]); // Liga o Led
    
    // Mantém LED aceso (tempo varia conforme dificuldade)
    delay(velocidade2);
    
    // Para o som e desliga o LED
    noTone(BUZZER);
    PORTD &= ~(1 << leds[sequencia[i]]);  // Desliga o Led

    // Intervalo entre LEDs (varia conforme dificuldade)
    delay(velocidade3);
  }
  
}

/*
  Aguarda o jogador repetir toda a sequência
  - Para cada passo da sequência, espera o jogador apertar um botão
  - Verifica se o botão pressionado está correto
  - Se errar, ativa a flag perdeuJogo e encerra
*/
void esperarJogador(){
  
  // Loop que aguarda cada passo da sequência ser repetido
  for (uint8_t i = 0; i < rodada; i++){
    bool jogou = false;
    
    // Fica em loop até o jogador apertar algum botão
    while (!jogou) {
      jogou = jogadaUsuario(); // Verifica se algum botão foi pressionado
    }
    
    // Após o jogador apertar, verifica se acertou
    // Se errou, a função retorna true e o break encerra o loop
    if(verificarJogada(i)) break;
    
    // Se acertou, avança para o próximo passo
    passo ++;
  }
  
  // Reseta o contador de passos para a próxima rodada
  passo = 0;
}

/*
  Reseta todas as variáveis e limpa a sequência para começar um novo jogo
  - Zera todo o array de sequência
  - Reseta contadores de rodada e passo
  - Desativa a flag de derrota
  - Reativa o menu de dificuldade
  - Restaura velocidades padrão (serão redefinidas ao selecionar dificuldade)
*/
void limparJogo(){

  // Limpa toda a sequência armazenada
  for(uint8_t i = 0; i < 12; i++){
    sequencia[i] = 0;
  }

    // Reseta variáveis de controle do jogo
    rodada = 0;
    passo = 0;
    perdeuJogo = false;

    /*
      Após o jogo terminar (vitória ou derrota):
      1. jogoAtivo = false → sai do modo de jogo
      2. menuDificuldadeAtivo = true → reativa o menu de seleção
      
      Isso permite ao jogador escolher nova dificuldade antes de jogar novamente
    */
    jogoAtivo = false;
    menuDificuldadeAtivo = false;
    SonsLeds = false;
    telaAtualizada = false;
    menuJogoAtivo = true;

    lcd.clear();

    /*
      Reseta os valores de velocidade (serão sobrescritos por aplicarDificuldade()
      quando jogador selecionar nova dificuldade)
    */
    velocidade1 = 0;
    velocidade2 = 0;
    velocidade3 = 0;
}

/*
  Verifica se algum botão foi pressionado e processa a jogada
  - Varre todos os 4 botões verificando se algum está pressionado
  - Quando detecta um pressionamento:
    * Armazena qual botão foi pressionado
    * Acende o LED e toca o som correspondente
    * Aguarda o botão ser solto (debounce) para evitar leituras múltiplas
  - Retorna true se um botão foi pressionado, false caso contrário
*/
bool jogadaUsuario() {

  // Loop que verifica cada um dos 4 botões
  for(uint8_t i = 0; i <= 3; i++){

    if(!(PINB & (1 << botoes[i]))){
      botaoPressionado = i; // Armazena qual botão foi pressionado
      
      // Feedback visual e sonoro ao jogador
      tone(BUZZER, tons[i], 250);
      PORTD |= (1 << leds[botaoPressionado]); // Liga o led
        
      delay(300);
      
      // Apaga o LED e para o som
      PORTD &= ~(1 << leds[botaoPressionado]); // Desliga o led
      noTone(BUZZER);

      // Debounce: aguarda o jogador soltar o botão
      // Evita que um único pressionamento seja contado múltiplas vezes
      while(!(PINB & (1 << botoes[i]))){
        delay(10);
      }

      // Retorna true indicando que uma jogada foi feita
      return true;
    }
  }

  // Se nenhum botão foi pressionado, retorna false
  return false;
}

/*
  Verifica se a jogada do usuário está correta
  - Compara o botão pressionado com o passo correto da sequência
  - Se errou: exibe mensagem de derrota, toca som de erro, pisca todos os LEDs e marca perdeuJogo
  - Retorna true se errou (para encerrar o loop), false se acertou
*/
bool verificarJogada(int index) {

  // Compara se o botão pressionado corresponde ao esperado na sequência
  if(sequencia[index] != botaoPressionado){

    visorDerrota(); // Exibe mensagem de derrota no LCD

    // Pisca todos os LEDs 3 vezes com som grave indicando erro
    for(uint8_t i = 0; i < 3; i++){

      tone(BUZZER, 70, 250);

      // Liga todos os 4 LEDs simultaneamente usando registrador
      PORTD |= ledsMascara;
      
      delay(200);
      
      noTone(BUZZER);

      // Desliga todos os 4 LEDs simultaneamente usando registrador
      PORTD &= ~ledsMascara;

      delay(200);
    }

    perdeuJogo = true;   // Marca que o jogo foi perdido
    return true;        // Retorna true para encerrar o loop de esperarJogador
  }

  // Se acertou, retorna false para continuar jogando
  return false;
}

/*
  Função chamada quando o jogador completa todas as 12 rodadas
  
  Reproduz um tema de vitória com show de luzes:
  - melodia[]: frequências das 13 notas
  - melodiaDuracao[]: duração de cada nota
  - melodiaPausa[]: pausa após cada nota
  
  Durante a melodia, todos os LEDs piscam rapidamente criando efeito estroboscópico
*/
void venceuJogo() {

  // ========== ARRAYS DA MELODIA DE VITÓRIA ==========
  // Array com as frequências das notas
  int melodia[] = {523, 659, 784, 1047, 784, 1047, 1319, 1047, 784, 659, 523, 392, 523};

  // Array com a duração de cada uma das 7 notas
  int melodiaDuracao[] = {150, 150, 150, 300, 150, 300, 400, 150, 150, 150, 300, 200,500};

  // Array com a pausa de cada uma das 7 notas
  int melodiaPausa[] = {195, 195, 195, 390, 195, 390, 520, 195, 195, 195, 390, 260, 650};
 
  // Loop que percorre todas as 7 notas da melodia de vitória
  for(uint8_t i = 0; i < sizeof(melodia)/sizeof(melodia[0]); i++){

    // Toca a nota atual com sua duração específica
    tone(BUZZER, melodia[i], melodiaDuracao[i]);

    // Acende todos os leds simultaneamente usando registrador
    PORTD |= ledsMascara;

    // LEDs ficam acesos por apenas 15ms (pisca muito rápido - efeito estroboscópico)
    delay(15);

    // Apaga todos os leds usando registrador
    PORTD &= ~ledsMascara;

    // Pausa após a nota (define o ritmo da música)
    delay(melodiaPausa[i]);

    // Para o som antes da próxima nota
    noTone(BUZZER);
  }

  // Após a melodia, o jogo será resetado pelo loop principal
}

/*
  Função chamada quando o jogador pressiona o botão iniciar
  
  Cria uma animação visual e sonora de boas-vindas que:
  - Aumenta progressivamente a frequência do som (efeito "power up")
  - Pisca todos os LEDs sincronizados com o som
  - Indica ao jogador que o jogo está prestes a começar
*/
void animacaoInicio() {
  /*
    Loop que cria sequência ascendente de 5 notas
    - f começa em 300 Hz (nota grave)
    - incrementa 100 Hz a cada iteração
    - termina em 700 Hz (nota mais aguda)
    
    Iterações: 300, 400, 500, 600, 700 Hz
  */
  for (int f = 300; f < 800; f += 100) {
     // Toca a frequência atual por 100ms
    tone(BUZZER, f, 100);

    // Acende todos os leds simultaneamente usando registrador
    PORTD |= ledsMascara;

    // Mantém LEDs acesos por 100ms (sincronizado com o som)
    delay(100);

    // Apaga todos os leds usando registrador
    PORTD &= ~ledsMascara;

    // Pausa de 100ms entre cada nota (cria o ritmo da animação)
    delay(100);
  }
  noTone(BUZZER);

  // Pausa de 1 segundo antes de iniciar o jogo
  // Dá tempo para o jogador se preparar mentalmente
  delay(1000);
}

/*
  Esta função é chamada quando o jogador confirma a dificuldade no menu
  através do botão BTN_INICIAR. Ela configura os três parâmetros de velocidade
  que controlam o ritmo do jogo.
*/
void aplicarDificuldade(){
  switch(nivelDificuldade){
    case 0: // Fácil
      velocidade1 = 1500;
      velocidade2 = 400;
      velocidade3 = 250;
      break;
    case 1: // Médio
      velocidade1 = 1000;
      velocidade2 = 300;
      velocidade3 = 200;
      break;
    case 2: // Difícil
      velocidade1 = 700;
      velocidade2 = 200;
      velocidade3 = 150;
      break;
  }
}

void menuJogo() {
  
  static uint8_t ultimaOpcao = 255; 

  if (opcaoMenu != ultimaOpcao) {
    if (opcaoMenu == 0) {
      lcd.setCursor(0, 0); lcd.print("> JOGO");
      lcd.setCursor(0, 1); lcd.print("  SONS");
    } else {
      lcd.setCursor(0, 0); lcd.print("  JOGO");
      lcd.setCursor(0, 1); lcd.print("> SONS");
    }
    ultimaOpcao = opcaoMenu;
  }

  // Navegação no Menu
  if (!(PINB & (1 << BTN_MENU))) {
    while (!(PINB & (1 << BTN_MENU))) { delay(10); }
    opcaoMenu = !opcaoMenu;
  }

  // Confirmação
  if (!(PIND & (1 << BTN_INICIAR))) {
    while (!(PIND & (1 << BTN_INICIAR))) { delay(10); }
    
    // Antes de mudar de estado, resetamos a sinalização
    ultimaOpcao = 255; 
    telaAtualizada = false;
    lcd.clear(); // Limpa o menu para entrar no próximo modo

    if (opcaoMenu == 0) {
      menuDificuldadeAtivo = true;
      menuJogoAtivo = false;
    } else {
      SonsLeds = true;
      menuJogoAtivo = false;
    }
  }
}

/* 
  Interface interativa no LCD que permite ao jogador escolher o nível de dificuldade

  FUNCIONALIDADE:
  - Exibe "Dificuldade:" na linha 1 do LCD
  - Mostra o nível atual com indicador "> " na linha 2
  - BTN_MENU: navega entre FACIL → MEDIO → DIFICIL → FACIL (cíclico)
  - BTN_INICIAR: confirma seleção e inicia o jogo
*/
void menuDificuldade(){

  if (!telaAtualizada) {
    lcd.setCursor(0, 0);
    lcd.print("Dificuldade:");
    telaAtualizada = true;
  }
  
  // Array constante com os textos de cada nível de dificuldade
  const char* niveis[3] = {"FACIL", "MEDIO", "DIFICIL"};

  //Inicializa o display do menu de dificuldade
  lcd.clear();               // Limpa o display 
  lcd.setCursor(0, 0);       // Move o cursor para a linha 1, coluna 0
  lcd.print("Dificuldade:"); // Exibe título na linha 1

  /*
    Este loop continua executando enquanto menuDificuldadeAtivo for true

    Dentro do loop:
    1. Atualiza display com nível atual
    2. Verifica se BTN_MENU foi pressionado (para navegar)
    3. Verifica se BTN_INICIAR foi pressionado (para confirmar)

    O loop só termina quando jogador pressiona BTN_INICIAR,
    que define menuDificuldadeAtivo = false e inicia o jogo
  */
  while(menuDificuldadeAtivo){

    // Atualiza a linha 2 do display com o nível selecionado
    lcd.setCursor(0, 1);                 // Move o cursor para a linha 2, coluna 0
    lcd.print("> ");                     // Indicador de seleção
    lcd.print(niveis[nivelDificuldade]); // Exibe o nível atual
    lcd.print("       ");                // Limpa o resto da linha         

    // Verifica se o botão de menu foi pressionado para mudar o nível
    if(!(PINB & (1 << BTN_MENU ))){
      
      // Debounce - aguarda o botão ser solto
      while(!(PINB & (1 << BTN_MENU ))){
        delay(10);
      }
      
      // Incrementa o nível de dificuldade (cíclico)
      nivelDificuldade++;

      /*
        Navegação cíclica

        Se passar de 2 (DIFÍCIL), volta para 0 (FÁCIL)
        Permite navegar infinitamente: FACIL → MEDIO → DIFICIL → FACIL → ...
      */
      if(nivelDificuldade > 2) nivelDificuldade = 0;
    }

    // Quando BTN_INICIAR é pressionado, inicia o jogo com a dificuldade selecionada
    if(!(PIND & (1 << BTN_INICIAR  ))){

      // Debounce - aguarda o botão ser solto
      while(!(PIND & (1 << BTN_INICIAR  ))){
        delay(10);
      }

      // Define velocidade1, velocidade2 e velocidade3 baseado em nivelDificuldade
      aplicarDificuldade();

      // Sai do loop while, permitindo que o jogo inicie
      menuDificuldadeAtivo = false;

      //Feedback visual no LCD indicando o nível selecionado
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Nivel: ");
      lcd.print(niveis[nivelDificuldade]);

      // Reproduz animação visual e sonora indicando início do jogo
      // Sequência de 5 notas ascendentes (300-700 Hz) com LEDs piscando
      animacaoInicio();

      // Muda o estado para jogoAtivo = true
      // Na próxima iteração do loop(), jogoGenius() será executado
      jogoAtivo = true;
    }

    // Pequeno delay para evitar leituras muito rápidas do botão
    delay(100);
  }
}

void ouvirLeds(){

  if(!telaAtualizada){
    visorSonsLeds();
    telaAtualizada = true;
  }

  for(uint8_t i = 0; i <= 3; i++){

    if(!(PINB & (1 << botoes[i]))){
      botaoPressionado = i; // Armazena qual botão foi pressionado
      
      // Feedback visual e sonoro ao jogador
      tone(BUZZER, tons[i], 250);
      PORTD |= (1 << leds[botaoPressionado]); // Liga o led
        
      delay(300);
      
      // Apaga o LED e para o som
      PORTD &= ~(1 << leds[botaoPressionado]); // Desliga o led
      noTone(BUZZER);

      // Debounce: aguarda o jogador soltar o botão
      while(!(PINB & (1 << botoes[i]))){
        delay(10);
      }
    }

    // Verifica se o botão de menu foi pressionado
    if(!(PINB & (1 << BTN_MENU ))){

      // Debounce: aguarda o jogador soltar o botão
      while(!(PINB & (1 << botoes[i]))){
        delay(10);
      }

      limparJogo();
    }
  }
}

/*
  Interface no LCD que exibe a rodada atual do jogo
  - Atualiza a linha 2 do LCD com o texto "Rodada: X"
  - Onde X é o número da rodada atual (1 a 12)
*/
void visorRodadas(){
  lcd.setCursor(0, 1);     // Linha 2
  lcd.print("Rodada: ");
  lcd.print(rodada);
  lcd.print("   ");       // Limpa possíveis restos
}

/*
  Interfaces no LCD que exibe mensagem de vitória
  - visorVitoria(): Exibe "Voce VENCEU!" e "Parabens!!!"
*/
void visorVitoria(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  Voce VENCEU!");
  lcd.setCursor(0, 1);
  lcd.print("  Parabens!!!");
}

/*
  Interfaces no LCD que exibe mensagem de derrota
  - visorDerrota(): Exibe "Voce PERDEU!" e "Tente Novamente"
*/
void visorDerrota(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  Voce PERDEU!");
  lcd.setCursor(0, 1);
  lcd.print("Tente Novamente!");
}

void visorSonsLeds() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  PARA VOLTAR!");
  lcd.setCursor(0, 1);
  lcd.print("APERTE BTN MENU!");
}