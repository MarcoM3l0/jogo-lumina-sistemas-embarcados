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
int sequencia[12] = {};

// Array com os pinos dos botões na mesma ordem dos LEDs
// Facilita o acesso aos botões através de índices
// Marcado como const para economizar RAM
const int botoes[4] = {BTN_VERMELHO, BTN_AMARELO, BTN_AZUL, BTN_VERDE};

// Array com os pinos dos LEDs na mesma ordem
// Permite acender qualquer LED usando: leds[numero]
// Marcado como const para economizar RAM
const int leds[4] = {LED_VERMELHO, LED_AMARELO, LED_AZUL, LED_VERDE};

// Array com as frequências de som para cada LED
// Cada LED toca uma nota musical diferente: RÉ, FÁ, MI, DÓ
// Marcado como const para economizar RAM
const int tons[4] = {294, 349, 330, 262};

// ========== VARIÁVEIS DE CONTROLE DO JOGO ==========
/*
  Contador de rodadas:
  - indica quantos passos já foram adicionados à sequência
  - Quando chega a 12, o jogador venceu o jogo
*/
int rodada = 0;

/*
  Contador de passos:
  - usado para verificar em qual posição da sequência o jogador está
  - Incrementado a cada acerto do jogador
*/
int passo = 0;

// Armazena qual botão foi pressionado pelo jogador
int botaoPressionado = 0;

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

// ========== VARIÁVEIS DE VELOCIDADE/DIFICULDADE ==========
/*
  Sistema de dificuldade progressiva:
  - velocidade1: delay entre rodadas (quanto tempo espera antes da próxima rodada)
  - velocidade2: tempo que o LED fica aceso ao reproduzir a sequência
  - velocidade3: intervalo entre LEDs na sequência
  
  Quanto menores os valores, mais rápido e difícil fica o jogo
*/
int velocidade1 = 1000; // Delay entre rodadas
int velocidade2 = 300;  // LED aceso na sequência
int velocidade3 = 200;  // Intervalo entre LEDs

void setup()
{
  // Configura LEDs (PD2, PD3, PD4, PD5) como saída
  // Combina múltiplos bits usando OR (|) para configurar todos de uma vez
  DDRD |= (1 << LED_VERMELHO) | (1 << LED_AMARELO) | (1 << LED_AZUL) | (1 << LED_VERDE);
  
  // Configura o Buzzer (PD7) como saída
  DDRD |= (1 << BUZZER);
  
  // Configura botão iniciar (PD6) como entrada
  // &= ~(NOT) zera o bit, configurando como entrada
  DDRD &= ~(1 << BTN_INICIAR);

  // Configura botões de jogo (PB0, PB1, PB2, PB3) como entrada
  DDRB &= ~( (1 << BTN_VERMELHO) | (1 << BTN_AMARELO) | (1 << BTN_AZUL) | (1 << BTN_VERDE));
  
  // Inicializa o gerador de números aleatórios usando ruído da porta analógica
  // Isso garante que cada jogo terá uma sequência diferente
  randomSeed(analogRead(A0));
}

void loop()
{
  // Gerenciamento de estado do Jogo
  if(jogoAtivo){
    // Se o jogo está ativo, executa a lógica principal do Genius
    jogoGenius();
  }else if((PIND & (1 << BTN_INICIAR  )) && !jogoAtivo){
    // Se o jogo NÃO está ativo E o botão iniciar foi pressionado:

    // Debounce: aguarda o jogador soltar o botão
    // Evita que um único pressionamento seja contado múltiplas vezes
    while(PIND & (1 << BTN_INICIAR  )){
      delay(10);
    }

    // Ativa o jogo para começar as rodadas
    jogoAtivo = true;
    delay(1000);
  }
}


// ========== FUNÇÕES DO JOGO ==========

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
    return;
  }
  // Fluxo normal do jogo:
  proximaRodada();        // 1. Adiciona um novo passo à sequência
  reproduzirSequencia();  // 2. Mostra a sequência completa ao jogador
  esperarJogador();       // 3. Aguarda o jogador repetir a sequência
  
  // Verifica se o jogador errou durante esperarJogador()
  // Se errou, retorna imediatamente sem executar o resto do código
  if(perdeuJogo) return;

  // Delay entre rodadas
  delay(velocidade1);

  // Quando chegar na rodada 7, aumenta a dificuldade
  // diminuindo os tempos de espera (jogo fica mais rápido)
  if(rodada == 7){
    velocidade1 = 500;
    velocidade2 = 150;
    velocidade3 = 100;
  }

  // Se o jogador completou todas as 12 rodadas, ele venceu!
  if(rodada == 12){
    venceuJogo();       // Toca melodia de vitória com show de luzes
    perdeuJogo = true;  // Usa a mesma flag para resetar o jogo
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
  - Adiciona delays para tornar visível cada passo
*/
void reproduzirSequencia(){
  
  for(int i = 0; i < rodada; i++){
    
    // Toca o som correspondente ao LED atual
    tone(BUZZER, tons[sequencia[i]], 250);
    PORTD |= (1 << leds[sequencia[i]]); // Liga o Led
    
    delay(velocidade2);
    
    noTone(BUZZER);
    PORTD &= ~(1 << leds[sequencia[i]]);  // Desliga o Led
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
  
  for (int i = 0; i < rodada; i++){
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
  - Adiciona pausa antes de recomeçar
*/
void limparJogo(){

  // Limpa toda a sequência armazenada
  for(int i = 0; i < 12; i++){
    sequencia[i] = 0;
  }

    // Reseta variáveis de controle do jogo
    rodada = 0;
    passo = 0;
    perdeuJogo = false;

    // Volta para o modo de espera até que o botão iniciar seja pressionado novamente
    jogoAtivo = false;

    // Volta a dificuldade para o nível inicial
    velocidade1 = 1000;
    velocidade2 = 300;
    velocidade3 = 200;
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
  for(int i = 0; i <= 3; i++){

    if(PINB & (1 << botoes[i])){
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
      while(PINB & (1 << botoes[i])){
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
  - Se errou: toca som de erro, pisca todos os LEDs e marca perdeuJogo
  - Retorna true se errou (para encerrar o loop), false se acertou
*/
bool verificarJogada(int index) {

  // Compara se o botão pressionado corresponde ao esperado na sequência
  if(sequencia[index] != botaoPressionado){

    // Animação de derrota: pisca todos os LEDs 3 vezes com som grave
    // Pisca todos os LEDs 3 vezes com som grave indicando erro
    for(int i = 0; i < 3; i++){
      tone(BUZZER, 70, 250);

      // Acende todos os 4 LEDs simultaneamente usando registrador
      for(int x = 0; x < 4; x++){
        PORTD |= (1 << leds[x]);  // Liga cada LED
      }
      
      delay(200);
      
      noTone(BUZZER);

      // Apaga todos os 4 LEDs usando registrador
      for(int x = 0; x < 4; x++){
        PORTD &= ~(1 << leds[x]); // Desliga cada LED
      }

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
  
  Reproduz uma versão abreviada do tema de Super Mario Bros com show de luzes:
  - melodia[]: array com as frequências das notas musicais
  - melodiaDuracao[]: duração de cada nota
  - melodiaPausa[]: pausa após cada nota
  
  Durante a melodia, todos os LEDs piscam rapidamente usando PORTD
  criando um efeito estroboscópico de celebração
*/
void venceuJogo() {

  // ========== ARRAYS DA MELODIA DE VITÓRIA ==========
  // Array com as frequências das 14 primeiras notas do tema do Super Mario Bros
  // Versão reduzida para economizar memória RAM do Arduino
  int melodia[] = {660, 660, 660, 510, 660, 770, 380, 510, 380, 320, 440, 480, 450, 430};

  // Array com a duração de cada nota
  // Controla por quanto tempo cada nota será tocada
  int melodiaDuracao[] = {100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 80, 100, 100};

  // Array com a pausa após cada nota
  // Controla o intervalo entre as notas para criar o ritmo
  int melodiaPausa[] = {150, 300, 300, 100, 300, 550, 575, 450, 400, 500, 300, 330, 150, 300};
 
  // Loop que percorre todas as 14 notas da melodia de vitória
  for(int i = 0; i < 14; i++){

    // Toca a nota atual com sua duração específica
    tone(BUZZER, melodia[i], melodiaDuracao[i]);

    // Acende todos os leds simultaneamente usando registrador
    PORTD |= (1 << LED_VERMELHO);
    PORTD |= (1 << LED_AMARELO);
    PORTD |= (1 << LED_AZUL);
    PORTD |= (1 << LED_VERDE);

    // LEDs ficam acesos por apenas 15ms (pisca muito rápido - efeito estroboscópico)
    delay(15);

    // Apaga todos os leds usando registrador
    PORTD &= ~(1 << LED_VERMELHO);
    PORTD &= ~(1 << LED_AMARELO);
    PORTD &= ~(1 << LED_AZUL);
    PORTD &= ~(1 << LED_VERDE);

    // Pausa após a nota (define o ritmo da música)
    delay(melodiaPausa[i]);

    // Para o som antes da próxima nota
    noTone(BUZZER);
  }

  // Após a melodia, o jogo será resetado pelo loop principal
}