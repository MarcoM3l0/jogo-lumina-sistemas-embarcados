// Variáveis Globais

// Definindo os leds
int ledVermelho = 2;
int ledAmarelo = 3;
int ledAzul = 4;
int ledVerde = 5;

// Definindo o Buzzer
int buzzer = 7;

// Definindo os botões
int btnVermelho = 8;
int btnAmarelo = 9;
int btnAzul = 10;
int btnVerde = 11;

/*
	Definindo os arrays
    - Sequencia
    	- Os números dentro dos índices representam: 
          - 0 -> Vermelho
          - 1 -> Amarelo
          - 2 -> Azul
          - 3 -> Verde
    - Botões
    - leds
    - tons (RÉ, FÁ, MI, DÓ)
*/
int sequencia[12] = {};
int botoes[4] = {btnVermelho, btnAmarelo, btnAzul, btnVerde};
int leds[4] = {ledVermelho, ledAmarelo, ledAzul, ledVerde};
int tons[4] = {294, 349, 330, 262};

// Variável de indicação da rodada
int rodada = 0;

// Variávei utilizadas para comparação 
// da sequencia sorteada com a sequencia do usuário
int passo = 0;
int botaoPressionado = 0;
bool perdeuJogo = false;

void setup()
{
  // Leds
  pinMode(ledVermelho, OUTPUT);
  pinMode(ledAmarelo, OUTPUT);
  pinMode(ledAzul, OUTPUT);
  pinMode(ledVerde, OUTPUT);
  
  //Buzzer
  pinMode(buzzer, OUTPUT);
  
  //Botões
  pinMode(btnVermelho, INPUT);
  pinMode(btnAmarelo, INPUT);
  pinMode(btnAzul, INPUT);
  pinMode(btnVerde, INPUT);
  
  // Monitor Serial: Debug
  Serial.begin(9600);
  
  
  // referência a inicialização da função random()
  randomSeed(analogRead(A0));
}

void loop()
{
  
  // Verificação da sequencia do usuário
  if(perdeuJogo == true){

    limparJogo();
  } else{

    proximaRodada();
    reproduzirSequencia();
    esperarJogador();
    
    delay(1000);
  }
}


// Métodos utilizados

void proximaRodada(){
  sequencia[rodada] = random(4);
  rodada += 1;
}

void reproduzirSequencia(){
  
  for(int i = 0; i < rodada; i++){
    
    tone(7, tons[sequencia[i]], 250);
    digitalWrite(leds[sequencia[i]], 1);
    
    delay(500);
    
    noTone(7);
    digitalWrite(leds[sequencia[i]], 0);
    delay(100);
  }
  
}

void esperarJogador(){
  
  for (int i = 0; i < rodada; i++){
    bool jogou = false;
    
    while (!jogou) {
      jogou = jogadaUsuario();
    }
    
    // Verificar a jogada
    if(verificarJogada(i)) break;
    
    passo += 1;
    
  }
  
  passo = 0;
}

void limparJogo(){

  for(int i = 0; i < 12; i++){
    sequencia[i] = 0;
  }

    rodada = 0;
    passo = 0;
    perdeuJogo = false;
}

bool jogadaUsuario() {

  for(int i = 0; i < 3; i++){
    if(digitalRead(botoes[i]) == 1){
      botaoPressionado = i;
      
      tone(7, tons[i], 250);
      digitalWrite(leds[i], 1);
        
      delay(300);
      
      digitalWrite(leds[botaoPressionado], 0);
      noTone(7);

      while(digitalRead(botoes[i]) == 1){
        delay(10);
      }

      return true;
    }
  }

  return false;
}


bool verificarJogada(int index) {

  if(sequencia[index] != botaoPressionado){
    for(int i = 0; i <= 3; i++){
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

    perdeuJogo = true;
    return true;
  }

  return false;
}









