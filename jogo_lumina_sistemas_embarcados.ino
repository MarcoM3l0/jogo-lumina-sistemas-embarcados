/*
  =====================================================
  LUMINA - Jogo de Memória com Luzes e Sons
  Desenvolvido para sistemas embarcados com Arduino

  Autores: José Marco Melo Nascimento
           Diogo Rodrigues da Silva
  =====================================================
*/

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

// Botão de navegação do menu (Port B)
// Cicla entre as 5 opções disponíveis: LIVRE → FACIL → MEDIO → DIFICIL → INSANO → LIVRE
// A cada pressionamento, avança para a próxima opção de forma cíclica
#define BTN_MENU     PB4 // Pino 12 (bit 4 do PORTB)

// ========== ESTRUTURAS DE DADOS ==========
/*
  Array que armazena a sequência do jogo
  O tamanho máximo varia conforme o nível selecionado:
  - FÁCIL:   até 10 rodadas
  - MÉDIO:   até 15 rodadas
  - DIFÍCIL: até 20 rodadas
  - INSANO:  até 50 rodadas (limite do array)
  Cada posição guarda um número de 0 a 3 que representa:
    0 -> LED Vermelho
    1 -> LED Amarelo
    2 -> LED Azul
    3 -> LED Verde
  Exemplo: {0, 2, 1, 2, ...} = Vermelho -> Azul -> Amarelo -> Azul -> ...
*/
uint8_t sequencia[50] = {};

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
  - Indica quantos passos já foram adicionados à sequência
  - O limite para vencer varia por nível (10, 15, 20 ou 50 rodadas)
*/
uint8_t rodada = 0;

/*
  Define o número de rodadas necessárias para vencer a partida.
  O valor é atribuído por aplicarDificuldade() conforme o nível:
  - FÁCIL:   10 rodadas
  - MÉDIO:   15 rodadas
  - DIFÍCIL: 20 rodadas
  - INSANO:  50 rodadas (teto de segurança do array; o desafio real é sobreviver)

  Resetado para 0 por limparJogo() ao encerrar a partida
*/
uint8_t limiteVitoria = 0;

/*
  Define em qual rodada as velocidades do jogo são reduzidas em 20%,
  criando um "salto" de dificuldade personalizado para cada nível:
  - FÁCIL:   acelera na rodada 5
  - MÉDIO:   acelera na rodada 7
  - DIFÍCIL: acelera na rodada 10
  - INSANO:  não utiliza este mecanismo (velocidade é sorteada a cada rodada)

  Atribuído por aplicarDificuldade() e resetado para 0 por limparJogo()
*/
uint8_t pontoDeAceleracao = 0;

// Armazena qual botão foi pressionado pelo jogador
uint8_t botaoPressionado = 0;

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

/*
  Flag que controla se o modo de exploração de sons e LEDs está ativo
  
  Estados:
  - true: Modo livre ativo — jogador pode pressionar os botões para
          ouvir as notas e acender os LEDs correspondentes livremente
  - false: Modo livre inativo
  
  Ativado no menuDificuldade() quando jogador seleciona a opção LIVRE
  Desativado por limparJogo() ao pressionar BTN_MENU para voltar ao menu
*/
bool modoLivreAtivo = false;

/*
  Flag de controle de atualização do LCD
  
  - false: A tela precisa ser redesenhada (estado mudou ou ainda não foi exibida)
  - true: A tela já foi atualizada para o estado atual, evita redesenhos desnecessários
  
  Resetada para false sempre que o estado do sistema muda (via atualizarTela())
  Definida como true dentro de cada função de exibição após o primeiro desenho
*/
bool telaAtualizada = false;

/*
  Armazena o estado de tela exibido anteriormente
  Usado pela função atualizarTela() para detectar mudanças de estado e
  forçar o redesenho do LCD apenas quando necessário

  Valores possíveis (espelham os argumentos passados em loop()):
  - 0: Jogo em andamento     → atualizarTela(0) antes de jogoLumina()
  - 1: Modo livre ativo      → atualizarTela(1) antes de modoLivre()
  - 2: Menu (seleção de modo)→ atualizarTela(2) antes de menuDificuldade()

  Inicializado com 255 (valor inválido) para garantir que o primeiro
  estado seja sempre desenhado ao ligar o sistema
*/
uint8_t estadoAnterior = 255;

// ========== SISTEMA DE DIFICULDADE ==========
/*
  Sistema com 5 opções de modo, navegadas pelo menu e aplicadas por aplicarDificuldade():

  LIVRE (nivelDificuldade = 0):
  - Ativa modoLivreAtivo = true; não inicia partida

  FÁCIL (nivelDificuldade = 1):
  - delayEntreRodadas: 1500ms | duracaoLedSequencia: 400ms | intervaloEntreLeds: 250ms
  - Vence em 10 rodadas | Acelera 20% na rodada 5

  MÉDIO (nivelDificuldade = 2):
  - delayEntreRodadas: 1000ms | duracaoLedSequencia: 300ms | intervaloEntreLeds: 200ms
  - Vence em 15 rodadas | Acelera 20% na rodada 7

  DIFÍCIL (nivelDificuldade = 3):
  - delayEntreRodadas: 700ms  | duracaoLedSequencia: 200ms | intervaloEntreLeds: 150ms
  - Vence em 20 rodadas | Acelera 20% na rodada 10

  INSANO (nivelDificuldade = 4):
  - Começa com os valores do FÁCIL; a cada rodada sorteia aleatoriamente um dos
    4 perfis (Fácil/Médio/Difícil/Insano) via aplicarVelocidadeModoInsano()
  - Perfil Insano puro: delayEntreRodadas 500ms, duracaoLedSequencia 100ms, intervaloEntreLeds 200ms
  - limiteVitoria = 50 (teto de segurança); pontoDeAceleracao = 255 (nunca dispara)
  - Sem condição de vitória real — o jogador joga até errar

  Quanto menores os valores de velocidade, mais rápido e difícil fica o jogo
*/
int delayEntreRodadas   = 0; // Delay entre rodadas (será definido por aplicarDificuldade())
int duracaoLedSequencia  = 0; // LED aceso na sequência (será definido por aplicarDificuldade())
int intervaloEntreLeds = 0; // Intervalo entre LEDs (será definido por aplicarDificuldade())

/*
  Armazena a opção selecionada no menu, usada por aplicarDificuldade()
  para configurar o modo de jogo ou ativar o modo livre

  Valores possíveis:
  - 0 = LIVRE   (ativa modoLivreAtivo — sem partida, exploração livre de sons e LEDs)
  - 1 = FÁCIL   (velocidades lentas,  acelera na rodada 5,  vence em 10 rodadas)
  - 2 = MÉDIO   (velocidades padrão,  acelera na rodada 7,  vence em 15 rodadas)
  - 3 = DIFÍCIL (velocidades rápidas, acelera na rodada 10, vence em 20 rodadas)
  - 4 = INSANO  (velocidade aleatória a cada rodada, sem condição de vitória real)

  Alterado pelo BTN_MENU em menuDificuldade() e aplicado ao pressionar BTN_INICIAR
*/
uint8_t nivelDificuldade = 0; 


/*
  Flag que controla se o menu principal está ativo

  Estados:
  - true:  Menu visível no LCD — jogador navega com BTN_MENU e confirma com BTN_INICIAR
           Inicializado como true pois menuDificuldade() é agora o ponto de entrada
           do sistema, exibido ao ligar e após encerrar qualquer partida ou modo livre
  - false: Menu fechado — jogo em andamento (jogoAtivo = true)
           Resetado de volta para true por limparJogo() ao encerrar a partida
*/
bool menuDificuldadeAtivo = true;

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
    O jogo opera em 3 estados mutuamente exclusivos,
    controlados pelas flags jogoAtivo e modoLivreAtivo
    (menuDificuldade() é executado no else, quando ambas são false):

    ESTADO 1 - JOGO ATIVO (jogoAtivo = true):
    - Executa a lógica principal do Lumina
    - Gerencia rodadas, sequências e validações
    - Detecta vitória ou derrota

    ESTADO 2 - MODO LIVRE (modoLivreAtivo = true):
    - Permite explorar sons e LEDs livremente
    - BTN_MENU retorna ao menu via limparJogo()

    ESTADO 3 - MENU (else — menuDificuldadeAtivo = true):
    - Ponto de entrada ao ligar e após encerrar qualquer partida ou modo livre
    - Exibe as 5 opções: LIVRE, FACIL, MEDIO, DIFICIL, INSANO
    - BTN_MENU navega entre as opções; BTN_INICIAR confirma

    Transições:
    - Menu → Modo Livre:  BTN_INICIAR com LIVRE selecionado (nivelDificuldade == 0)
    - Menu → Jogo:        BTN_INICIAR com qualquer nível 1–4 selecionado
    - Jogo → Menu:        partida encerrada (vitória ou derrota) via limparJogo()
    - Modo Livre → Menu:  BTN_MENU pressionado em modoLivre() via limparJogo()
  */
  if (jogoAtivo) {
    atualizarTela(0);
    jogoLumina();
  }  
  else if (modoLivreAtivo) {
    atualizarTela(1);
    modoLivre();
  }
  else{
    atualizarTela(2);
    menuDificuldade();
  }
}


// ========== FUNÇÕES DO JOGO ==========

/*
  Monitora mudanças de estado do sistema e sinaliza quando o LCD precisa
  ser redesenhado.
  
  Compara o estado atual com o anterior (estadoAnterior):
  - Se forem diferentes: reseta telaAtualizada = false, forçando o redesenho
    do LCD na próxima execução da função de exibição correspondente,
    e atualiza estadoAnterior com o novo valor
  - Se forem iguais: não faz nada, evitando redesenhos desnecessários
  
  Chamada no início de cada iteração do loop(), antes de executar
  a função do estado ativo (menuDificuldade, modoLivre ou jogoLumina)
*/
void atualizarTela(uint8_t estadoAtual) {
  if (estadoAtual != estadoAnterior) {
    telaAtualizada = false; 
    estadoAnterior = estadoAtual;
  }
}

/*
  Lógica principal do Jogo Lumina.
  Gerencia todo o fluxo de uma partida do jogo:
  - Adiciona novos passos à sequência
  - Reproduz a sequência para o jogador
  - Aguarda e valida as jogadas do jogador
  - Controla progressão de dificuldade
  - Detecta vitória ou derrota
  
  Esta função só é executada quando jogoAtivo == true
*/
void jogoLumina(){

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
  delay(delayEntreRodadas);

  /*
    Progressão de dificuldade após cada rodada:

    MODO INSANO (nivelDificuldade == 4):
    - A cada rodada, sorteia aleatoriamente um dos 4 perfis de velocidade
      (Fácil, Médio, Difícil ou Insano) via aplicarVelocidadeModoInsano()
    - Isso torna o ritmo imprevisível, podendo acelerar ou desacelerar
      a qualquer momento, exigindo adaptação constante do jogador

    DEMAIS MODOS (rodada == pontoDeAceleracao):
    - Ao atingir o pontoDeAceleracao definido pelo nível selecionado,
      todas as velocidades são reduzidas em 20%, criando um "salto" de
      dificuldade personalizado:
        FÁCIL   → acelera na rodada 5
        MÉDIO   → acelera na rodada 7
        DIFÍCIL → acelera na rodada 10
  */
  if(nivelDificuldade == 4){
   uint8_t nivelAleatorio = random(4); // Sorteia um perfil de velocidade entre 0 e 3
   aplicarVelocidadeModoInsano(nivelAleatorio);
  }
  else if(rodada == pontoDeAceleracao){
    delayEntreRodadas -= (delayEntreRodadas * 20) / 100; // Reduz 20% do valor atual
    duracaoLedSequencia -= (duracaoLedSequencia * 20) / 100; // Reduz 20% do valor atual
    intervaloEntreLeds -= (intervaloEntreLeds * 20) / 100; // Reduz 20% do valor atual
  }

  /* 
    Verifica se o jogador atingiu o limite de rodadas para vencer.
    O limiteVitoria varia conforme o nível e é definido por aplicarDificuldade():
      FÁCIL   → 10 rodadas
      MÉDIO   → 15 rodadas
      DIFÍCIL → 20 rodadas
      INSANO  → 50 rodadas (teto de segurança do array; na prática o jogador
                            dificilmente chega lá — o desafio é sobreviver)

    Ao atingir o limite:
    - Exibe mensagem de vitória no LCD
    - Reproduz melodia de vitória com show de luzes
    - Ativa perdeuJogo = true para acionar o reset no próximo ciclo do loop
  */
  if(rodada == limiteVitoria){
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
    delay(duracaoLedSequencia);
    
    // Para o som e desliga o LED
    noTone(BUZZER);
    PORTD &= ~(1 << leds[sequencia[i]]);  // Desliga o Led

    // Intervalo entre LEDs (varia conforme dificuldade)
    delay(intervaloEntreLeds);
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
  }
}

/*
  Reseta todas as variáveis e limpa a sequência para começar um novo jogo
  - Zera todo o array de sequência
  - Reseta contadores de rodada e passo
  - Desativa as flags
  - Reativa o menu principal
  - Restaura velocidades padrão (serão redefinidas ao selecionar dificuldade)
*/
void limparJogo(){

  // Limpa toda a sequência armazenada
  for(uint8_t i = 0; i < 50; i++){
    sequencia[i] = 0;
  }

  // Reseta variáveis de controle do jogo
  rodada = 0;
  limiteVitoria = 0;      // Resetado aqui pois seu valor depende do nível selecionado
  pontoDeAceleracao = 0;  // Idem — será redefinido por aplicarDificuldade() na próxima partida
  perdeuJogo = false;

  /*
    Após o jogo terminar (vitória ou derrota), todas as flags de estado
    são resetadas para retornar ao menu:

    1. jogoAtivo = false           → sai do modo de jogo
    2. menuDificuldadeAtivo = true → reativa o menu como ponto de entrada
    3. modoLivreAtivo = false      → garante que o modo livre não abra sozinho
    4. telaAtualizada = false      → força o redesenho do LCD ao entrar no menu

    Na próxima iteração do loop(), o else será executado,
    chamando menuDificuldade() e exibindo o menu novamente
  */
  jogoAtivo = false;
  menuDificuldadeAtivo = true;
  modoLivreAtivo = false;
  telaAtualizada = false;

  lcd.clear(); // Limpar o LCD

  /*
    Reseta os valores de velocidade (serão sobrescritos por aplicarDificuldade()
    quando jogador selecionar nova dificuldade)
  */
  delayEntreRodadas = 0;
  duracaoLedSequencia = 0;
  intervaloEntreLeds = 0;
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
bool verificarJogada(uint8_t index) {

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
  Chamada quando o jogador atinge o limiteVitoria do nível selecionado

  Reproduz um tema de vitória com show de luzes:
  - melodia[]: frequências das 13 notas
  - melodiaDuracao[]: duração de cada nota
  - melodiaPausa[]: pausa após cada nota

  Durante a melodia, todos os LEDs piscam rapidamente criando efeito estroboscópico
*/
void venceuJogo() {

  // ========== ARRAYS DA MELODIA DE VITÓRIA ==========
  // Array com as frequências das notas
  int melodia[] = {440, 494, 523, 587, 659, 587, 523, 659, 784, 659, 784, 880, 1047};

  // Array com a duração de cada uma das 13 notas
  int melodiaDuracao[] = {120, 120, 120, 120, 240, 120, 120, 180, 180, 120, 180, 240, 480};

  // Array com a pausa após cada uma das 13 notas
  int melodiaPausa[] = {156, 156, 156, 156, 312, 156, 156, 234, 234, 156, 234, 312, 624};
 
  // Loop que percorre todas as 13 notas da melodia de vitória
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
  Chamada quando o jogador confirma a seleção no menu via BTN_INICIAR.
  Configura os parâmetros da partida conforme a opção escolhida:

  case 0 (LIVRE):   ativa modoLivreAtivo = true — não configura velocidades nem inicia jogo
  case 1 (FÁCIL):   velocidades lentas,  limiteVitoria = 10, pontoDeAceleracao = 5
  case 2 (MÉDIO):   velocidades padrão,  limiteVitoria = 15, pontoDeAceleracao = 7
  case 3 (DIFÍCIL): velocidades rápidas, limiteVitoria = 20, pontoDeAceleracao = 10
  case 4 (INSANO):  velocidades iniciais do FÁCIL, limiteVitoria = 50,
                    pontoDeAceleracao = 255 (valor sentinela — nunca dispara o if de aceleração,
                    pois a velocidade é gerenciada rodada a rodada por aplicarVelocidadeModoInsano())
*/
void aplicarDificuldade(){
  switch(nivelDificuldade){
    case 0: // Livre
      modoLivreAtivo = true;
      break;
    case 1: // Fácil
      delayEntreRodadas = 1500;
      duracaoLedSequencia = 400;
      intervaloEntreLeds = 250;
      limiteVitoria = 10;
      pontoDeAceleracao = 5;
      break;
    case 2: // Médio
      delayEntreRodadas = 1000;
      duracaoLedSequencia = 300;
      intervaloEntreLeds = 200;
      limiteVitoria = 15;
      pontoDeAceleracao = 7;
      break;
    case 3: // Difícil
      delayEntreRodadas = 700;
      duracaoLedSequencia = 200;
      intervaloEntreLeds = 150;
      limiteVitoria = 20;
      pontoDeAceleracao = 10;
      break;
    case 4: // INSANO - Começa como o Fácil, mas muda dinamicamente a cada rodada
      delayEntreRodadas = 1500;
      duracaoLedSequencia = 400;
      intervaloEntreLeds = 250;
      limiteVitoria = 50;
      pontoDeAceleracao = 255; // Nunca acelera via 'if', pois usa o Caos Aleatório
      break;
  }
}

/*
  Aplica um perfil de velocidade aleatório no modo INSANO.
  Chamada a cada rodada com um índice sorteado entre 0 e 3,
  podendo aplicar qualquer um dos 4 perfis abaixo:

  - case 0 (Fácil):   ritmo lento, fácil de acompanhar
  - case 1 (Médio):   ritmo moderado
  - case 2 (Difícil): ritmo rápido
  - case 3 (Insano):  ritmo extremo — LEDs piscam muito rapidamente
                      e o intervalo entre eles é mínimo

  A imprevisibilidade entre os perfis é o que define o desafio do modo INSANO,
  pois o jogador não consegue se adaptar a um ritmo fixo
*/
void aplicarVelocidadeModoInsano(uint8_t nivelAleatorio){
  switch(nivelAleatorio){
    case 0: // Fácil
      delayEntreRodadas = 1500;
      duracaoLedSequencia = 400;
      intervaloEntreLeds = 250;
      break;
    case 1: // Médio
      delayEntreRodadas = 1000;
      duracaoLedSequencia = 300;
      intervaloEntreLeds = 200;
      break;
    case 2: // Difícil
      delayEntreRodadas = 700;
      duracaoLedSequencia = 200;
      intervaloEntreLeds = 150;
      break;
    case 3: // INSANO 
      delayEntreRodadas = 500;
      duracaoLedSequencia = 100;
      intervaloEntreLeds = 200;
      break;
  }
}

/* 
  Menu principal do sistema — centralizando a navegação 
  de modo e dificuldade em uma única função.

  FUNCIONALIDADE:
  - Exibe "Dificuldade:" na linha 1 do LCD
  - Mostra a opção atual com indicador "> " na linha 2
  - BTN_MENU: avança ciclicamente entre as 5 opções (índices 0 a 4):
              LIVRE → FACIL → MEDIO → DIFICIL → INSANO → LIVRE → ...
  - BTN_INICIAR: confirma a seleção e redireciona conforme o índice:
      0 (LIVRE)  → chama aplicarDificuldade() que ativa modoLivreAtivo = true,
                   depois retorna (return) sem fechar menuDificuldadeAtivo nem iniciar jogo
      1–4        → chama aplicarDificuldade(), fecha o menu, exibe animação e inicia jogo
*/
void menuDificuldade(){

  //Inicializa o display do menu de dificuldade
  if (!telaAtualizada) {
    lcd.clear();
    lcd.setCursor(0, 0);       // Move o cursor para a linha 1, coluna 0
    lcd.print(F("Dificuldade:")); // Exibe título na linha 1
  }
  
  // Array com as 5 opções do menu, indexadas por nivelDificuldade
  const char* niveis[5] = {"LIVRE", "FACIL", "MEDIO", "DIFICIL", "INSANO"};
  
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
        Navegação cíclica entre as 5 opções (índices 0 a 4)
        Se passar de 4 (INSANO), volta para 0 (LIVRE)
        Permite navegar infinitamente: LIVRE → FACIL → MEDIO → DIFICIL → INSANO → LIVRE → ...
      */
      if(nivelDificuldade > 4) nivelDificuldade = 0;
    }

    // Quando BTN_INICIAR é pressionado, inicia o jogo com a dificuldade selecionada
    if(!(PIND & (1 << BTN_INICIAR  ))){

      // Debounce - aguarda o botão ser solto
      while(!(PIND & (1 << BTN_INICIAR  ))){
        delay(10);
      }

      // Configura o modo selecionado: velocidades, limiteVitoria e pontoDeAceleracao
      // Para LIVRE (índice 0): apenas ativa modoLivreAtivo = true
      aplicarDificuldade();

      if(nivelDificuldade != 0){
        // Fecha o menu e inicia a partida
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
        // Na próxima iteração do loop(), jogoLumina() será executado
        jogoAtivo = true;
      } 
      // LIVRE selecionado: modoLivreAtivo já foi ativado em aplicarDificuldade(),
      // retorna sem iniciar jogo
      else {

        animacaoInicio();

        // Fecha o menu e inicia o modo Livre
        menuDificuldadeAtivo = false;
        return;
      }
    }

    // Pequeno delay para evitar leituras muito rápidas do botão
    delay(100);
  }
}

/*
  Modo de exploração livre de sons e LEDs
  
  FUNCIONALIDADE:
  - Permite ao jogador pressionar qualquer um dos 4 botões de cor
    para acender o LED correspondente e ouvir sua nota musical
  - Útil para aprender a associação entre cores, botões e sons
    antes de iniciar uma partida
  - BTN_MENU: encerra o modo e retorna ao menu principal via limparJogo()

  FLUXO:
  1. Na primeira execução, exibe instruções no LCD via visorModoLivre()
  2. Varre os 4 botões de cor continuamente
  3. Ao detectar pressionamento: acende LED, toca nota, aguarda soltar (debounce)
  4. Verifica BTN_MENU a cada ciclo — se pressionado, chama limparJogo()
     para resetar os estados e voltar ao menu principal
*/
void modoLivre(){

  // Exibe instruções no LCD apenas na primeira vez que entra no modo
  if(!telaAtualizada){
    visorModoLivre();
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
      while(!(PINB & (1 << BTN_MENU))){
        delay(10);
      }

      // Reseta todos os estados e retorna ao menu principal
      limparJogo();
    }
  }
}

/*
  Interface no LCD que exibe a rodada atual do jogo
  - Atualiza a linha 2 do LCD com o texto "Rodada: X"
  - Onde X é o número da rodada atual (varia conforme o limiteVitoria do nível)
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
  lcd.print(" Voce VENCEU!");
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

/*
  Interface no LCD que exibe as instruções do modo livre
  - Linha 1: instrução para voltar ao menu ("PARA VOLTAR!")
  - Linha 2: instrução do botão a pressionar ("APERTE BTN MENU!")
  
  Exibida apenas uma vez ao entrar no modo modoLivre(),
  controlada pela flag telaAtualizada
*/
void visorModoLivre() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  PARA VOLTAR!");
  lcd.setCursor(0, 1);
  lcd.print("APERTE BTN MENU!");
}