// ========== CONFIGURAÇÃO DE HARDWARE ==========
// Pinos dos LEDs
int ledVermelho = 2;
int ledAmarelo = 3;
int ledAzul = 4;
int ledVerde = 5;

// Pino do Buzzer
int buzzer = 7;

// Pinos dos Botões
int btnVermelho = 8;
int btnAmarelo = 9;
int btnAzul = 10;
int btnVerde = 11;

// Botão para iniciar o jogo
// Quando pressionado, ativa o modo de jogo
int btnIniciar = 6;

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
int botoes[4] = {btnVermelho, btnAmarelo, btnAzul, btnVerde};

// Array com os pinos dos LEDs na mesma ordem
// Permite acender qualquer LED usando: leds[numero]
int leds[4] = {ledVermelho, ledAmarelo, ledAzul, ledVerde};

// Array com as frequências de som para cada LED
// Cada LED toca uma nota musical diferente: RÉ, FÁ, MI, DÓ
int tons[4] = {294, 349, 330, 262};

// ========== VARIÁVEIS DE CONTROLE DO JOGO ==========
/*
  Contador de rodadas:
  - indica quantos passos já foram adicionados à sequência
*/
int rodada = 0;

/*
  Contador de passos:
  - usado para verificar em qual posição da sequência o jogador está
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
int velocidade1 = 1000;
int velocidade2 = 300;
int velocidade3 = 200;

void setup()
{
  // Configura todos os LEDs como saída 
  pinMode(ledVermelho, OUTPUT);
  pinMode(ledAmarelo, OUTPUT);
  pinMode(ledAzul, OUTPUT);
  pinMode(ledVerde, OUTPUT);
  
  // Configura o Buzzer como saída
  pinMode(buzzer, OUTPUT);
  
  // Configura todos os botões como entrada
  pinMode(btnVermelho, INPUT);
  pinMode(btnAmarelo, INPUT);
  pinMode(btnAzul, INPUT);
  pinMode(btnVerde, INPUT);
  pinMode(btnIniciar, INPUT);
  
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
  }else if((digitalRead(btnIniciar) == 1) && !jogoAtivo){
    // Se o jogo NÃO está ativo E o botão iniciar foi pressionado:

    // Debounce: aguarda o jogador soltar o botão
    // Evita que um único pressionamento seja contado múltiplas vezes
    while(digitalRead(btnIniciar)){
      delay(10);
    }

    // Ativa o jogo para começar as rodadas
    jogoAtivo = true;
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
    // Se perdeu ou venceu, reseta tudo e começa um novo jogo
    limparJogo();
  } else{
    // Fluxo normal do jogo:
    proximaRodada();        // 1. Adiciona um novo passo à sequência
    reproduzirSequencia();  // 2. Mostra a sequência completa ao jogador
    esperarJogador();       // 3. Aguarda o jogador repetir a sequência
    
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
}

/*
  Adiciona um novo passo aleatório à sequência
  - Sorteia um número de 0 a 3 (representando as 4 cores)
  - Adiciona na posição atual da rodada
  - Incrementa o contador de rodadas
*/
void proximaRodada(){
  sequencia[rodada] = random(4);
  rodada += 1;
}

/*
  Reproduz a sequência completa para o jogador
  - Percorre todos os passos da sequência atual
  - Para cada passo: acende o LED correspondente e toca seu som
  - Adiciona delays para tornar visível cada passo
*/
void reproduzirSequencia(){
  
  for(int i = 0; i < rodada; i++){
    
    tone(7, tons[sequencia[i]], 250);
    digitalWrite(leds[sequencia[i]], 1);
    
    delay(velocidade2);
    
    noTone(7);
    digitalWrite(leds[sequencia[i]], 0);
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
    passo += 1;
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

  for(int i = 0; i < 12; i++){
    sequencia[i] = 0;
  }

    rodada = 0;
    passo = 0;
    perdeuJogo = false;
    jogoAtivo = false;

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

  for(int i = 0; i <= 3; i++){
    if(digitalRead(botoes[i]) == 1){
      botaoPressionado = i; // Armazena qual botão foi pressionado
      
      tone(7, tons[i], 250);
      digitalWrite(leds[i], 1);
        
      delay(300);
      
      digitalWrite(leds[botaoPressionado], 0);
      noTone(7);

      // Debounce: aguarda o jogador soltar o botão
      // Evita que um único pressionamento seja contado múltiplas vezes
      while(digitalRead(botoes[i]) == 1){
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

  if(sequencia[index] != botaoPressionado){

    // Animação de derrota: pisca todos os LEDs 3 vezes com som grave
    for(int i = 0; i < 3; i++){
      tone(7, 70, 250);

      for(int x = 0; x < 4; x++){
        digitalWrite(leds[x], 1);
      }
      
      delay(200);
      
      noTone(7);
      for(int x = 0; x < 4; x++){
        digitalWrite(leds[x], 0);
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
  
  Reproduz uma melodia de vitória (tema do Super Mario Bros) com show de luzes:
  - melodia[]: array com as frequências das notas musicais
  - melodiaDuracao[]: duração de cada nota
  - melodiaPausa[]: pausa após cada nota
  
  Durante a melodia, todos os LEDs piscam sincronizados com a música
  criando um efeito visual de celebração
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
 
  // Reprodução da melodia e o show de luzes
  for(int i = 0; i < 14; i++){
    tone(7, melodia[i], melodiaDuracao[i]);

    digitalWrite(ledVermelho, 1);
    digitalWrite(ledAmarelo, 1);
    digitalWrite(ledAzul, 1);
    digitalWrite(ledVerde, 1);

    delay(15);

    digitalWrite(ledVermelho, 0);
    digitalWrite(ledAmarelo, 0);
    digitalWrite(ledAzul, 0);
    digitalWrite(ledVerde, 0);

    delay(melodiaPausa[i]);

    noTone(7);
  }

  // Após a melodia, o jogo será resetado pelo loop principal
  delay(2000);
}