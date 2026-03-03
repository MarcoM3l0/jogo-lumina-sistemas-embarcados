# Jogo da memória com Arduino Uno

Este repositório contém a implementação de um **jogo da memória** utilizando o **Arduino Uno**, desenvolvido como projeto final da disciplina **Introdução aos Sistemas Embarcados**.

O projeto foi inicialmente modelado, programado e testado no **Tinkercad**, permitindo a simulação do circuito eletrônico e a validação do código antes da implementação física.

---

## 🎯 Objetivo do Projeto

O objetivo deste projeto é aplicar conceitos fundamentais de **sistemas embarcados**, como:

- Leitura de entradas digitais
- Controle de saídas (LEDs e buzzer)
- Lógica de estados
- Temporização
- Interação homem-máquina

Tudo isso por meio da implementação de um jogo clássico amplamente utilizado em projetos didáticos.

---

## 🕹️ Descrição do Jogo da memória

O jogo da memória funciona da seguinte forma:

1. O sistema gera uma sequência aleatória de cores/luzes.
2. O jogador deve repetir a sequência pressionando os botões correspondentes.
3. A cada rodada, a sequência aumenta em dificuldade.
4. Caso o jogador erre a sequência, o jogo é reiniciado.

---

## ⚙️ Tecnologias e Ferramentas Utilizadas

- **Arduino Uno**
- **Linguagem C/C++ (Arduino)**
- **Tinkercad**
  - Simulação do circuito eletrônico
  - Escrita e testes do código
- Componentes eletrônicos:
  - Arduino UNO
  - LEDs
  - Botões
  - LCD I2C
  - Resistores
  - Buzzer

---

## 🔌 Esquema Eletrônico

O circuito foi montado no **Tinkercad**, conforme ilustrado abaixo:

![Esquema do circuito no Tinkercad](https://github.com/MarcoM3l0/jogo-lumina-sistemas-embarcados/blob/main/jogo-genius-sistemas-embarcados.png)

### Componentes Utilizados

- 1 × Arduino Uno
- 4 × LEDs (representando as cores do jogo)
- 6 × Botões
- 4 × Resistores para LEDs (220 Ω)
- 1 × Buzzer
- 1 × Display LCD 16x2 com interface I2C (PCF8574)
- Protoboard e jumpers

### Descrição do Circuito

- Cada **LED** representa uma cor do jogo da memória.
- Quatro **botães** corresponde a um LED específico.
- Um **botão** correspondente ao start do jogo.
- Um **botão** correspondente a navegação entre os níveis de dificuldade antes do início do jogo.
- O **display LCD 16x2 (I2C)** é responsável por exibir informações ao usuário, como menu de dificuldade, nível selecionado, rodada atual e estados do jogo.
- O **buzzer** fornece feedback sonoro para eventos como início de rodada, acerto, erro ou vitória.
- Os resistores são utilizados para limitar a corrente nos LEDs e garantir leituras corretas nos botões.

---

## 🧩 Funcionalidades

- Geração de sequência aleatória
- Exibição da sequência por LEDs
- Leitura de entradas dos botões
- Validação da sequência informada pelo jogador
- Aumento progressivo da dificuldade
- Feedback visual e/ou sonoro para acertos e erros

---

## 📚 Disciplina

- Projeto desenvolvido para a disciplina de Introdução aos Sistemas Embarcados.

---

## ✍️ Autor

- Marco Melo

---

## 📌 Observações Finais

- Este projeto possui caráter didático e experimental, com foco no aprendizado prático dos conceitos básicos de sistemas embarcados.
