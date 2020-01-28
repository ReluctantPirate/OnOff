bool isOn = false;
byte orientationFace = 0;

enum messages {INERT, REVERSE1, REVERSE2, REVERSE3, TURNON, TURNOFF, LISTENING};
byte messageState[6] = {INERT, INERT, INERT, INERT, INERT, INERT};
bool activelyMessaging = false;

#define TYPE_AMOUNT 5
enum blinkTypes {LIGHT, FIRE, ELEC, GRASS, WATER};
byte blinkType = LIGHT;

void setup() {

}

void loop() {
  //listen for clicks
  if (buttonSingleClicked()) {
    FOREACH_FACE(f) {
      messageState[f] = INERT;
    }

    switch (blinkType) {
      case LIGHT:
        sendLightSignal();
        break;
      case FIRE:
        sendFireSignal();
        break;
      case ELEC:
        sendElecSignal();
        break;
      case GRASS:
        sendGrassSignal();
        break;
      case WATER:
        sendWaterSignal();
        break;
    }
    isOn = !isOn;
    orientationFace = (orientationFace + 1) % 6;
  }

  if (buttonDoubleClicked()) {
    isOn = false;
    blinkType = (blinkType + 1) % TYPE_AMOUNT;
  }

  //listen on all faces for messaging
  FOREACH_FACE(f) {
    switch (messageState[f]) {
      case INERT:
        inertLoop(f);
        break;
      case LISTENING:
        listeningLoop(f);
        break;
      default:
        activeMessagingLoop(f);
        break;
    }
  }

  //set communication
  FOREACH_FACE(f) {
    byte sendData = (isOn << 5) + (messageState[f]);
    setValueSentOnFace(sendData, f);
  }

  //do display
  byte dimness = 100;
  if (isOn) {
    dimness = 255;
  }

  FOREACH_FACE(f) {
    switch (messageState[f]) {
      case INERT:
        setColorOnFace(WHITE, f);
        break;
      case REVERSE1:
        setColorOnFace(YELLOW, f);
        break;
      case REVERSE2:
        setColorOnFace(ORANGE, f);
        break;
      case REVERSE3:
        setColorOnFace(RED, f);
        break;
      case TURNON:
        setColorOnFace(GREEN, f);
        break;
      case TURNOFF:
        setColorOnFace(BLUE, f);
        break;
      case LISTENING:
        setColorOnFace(MAGENTA, f);
        break;
    }
  }
  //  switch (blinkType) {
  //    case LIGHT:
  //      lightDisplay(dimness);
  //      break;
  //    case FIRE:
  //      fireDisplay(dimness);
  //      break;
  //    case ELEC:
  //      elecDisplay(dimness);
  //      break;
  //    case GRASS:
  //      grassDisplay(dimness);
  //      break;
  //    case WATER:
  //      waterDisplay(dimness);
  //      break;
  //  }
}

void sendLightSignal() {//reverse all neigbors
  FOREACH_FACE(f) {
    messageState[f] = REVERSE1;
  }
}

void sendFireSignal() {//a Y of reversals
  messageState[orientationFace] = REVERSE2;
  messageState[(orientationFace + 2) % 6] = REVERSE2;
  messageState[(orientationFace + 4) % 6] = REVERSE2;
}

void sendElecSignal() {//a long line or reversals
  messageState[orientationFace] = REVERSE3;
  messageState[(orientationFace + 3) % 6] = REVERSE3;
}

void sendGrassSignal() {//an arc of reversals
  messageState[orientationFace] = REVERSE2;
  messageState[(orientationFace + 1) % 6] = REVERSE2;
  messageState[(orientationFace + 2) % 6] = REVERSE2;
}

void sendWaterSignal() {//this one is complicated actually
  if (!isAlone) {//only do anything if you have neighbors
    //gather all neighbor data
    byte neighborStates[6];//0 is no neighbor, 1 is neighbor
    byte activeNeighbors = 0;
    byte neighborBitwise = 0;
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {//neighbor
        neighborStates[f] = 1;
        neighborBitwise = (neighborBitwise << 1) + getIsOn(getLastValueReceivedOnFace(f));
        activeNeighbors++;
      } else {
        neighborStates[f] = 0;
      }
    }

    //rotate neighbor data
    byte lastDigit = neighborBitwise & 1;
    neighborBitwise = (neighborBitwise >> 1) + (lastDigit << (activeNeighbors - 1));

    //place in neighborStates array
    FOREACH_FACE(f) {
      if (neighborStates[f] == 1) {
        activeNeighbors--;
        neighborStates[f] = (neighborBitwise >> activeNeighbors) & 1;//just gets the 1 digit we need
      }
    }

    //send messages on those faces
    FOREACH_FACE(f) {
      if (neighborStates[f] == 1) {
        messageState[f] = TURNON;
      } else {
        messageState[f] = TURNOFF;
      }
    }
  }
}

void inertLoop(byte face) {
  if (!isValueReceivedOnFaceExpired(face)) {//there is something at this face
    byte neighborData = getLastValueReceivedOnFace(face);
    if (getMessageState(neighborData) != INERT && getMessageState(neighborData) != LISTENING) {//this face is actively communicating something to me
      messageState[face] = LISTENING;
      switch (getMessageState(neighborData)) {
        case REVERSE1:
          doReverse(1, face);
          break;
        case REVERSE2:
          doReverse(1, face);
          break;
        case REVERSE3:
          doReverse(1, face);
          break;
        case TURNON:
          isOn = true;
          break;
        case TURNOFF:
          isOn = false;
          break;
      }
    }
  }
}

void listeningLoop(byte face) {
  if (!isValueReceivedOnFaceExpired(face)) {//there is something at this face
    if (getMessageState(getLastValueReceivedOnFace(face)) == INERT) {//this face has resolved, so you can too
      messageState[face] = INERT;
    }
  } else {//no neighbor, so just go INERT
    messageState[face] = INERT;
  }
}

void activeMessagingLoop(byte face) {
  if (!isValueReceivedOnFaceExpired(face)) {//there is something at this face
    if (getMessageState(getLastValueReceivedOnFace(face)) == LISTENING) {//this face has heard you, so you can resolve
      messageState[face] = INERT;
    }
  } else {//no neighbor, so just go INERT
    messageState[face] = INERT;
  }
}

void doReverse(byte num, byte face) {
  isOn = !isOn;
  if (num == 3) {
    messageState[oppositeFace(face)] = REVERSE2;
  } else if (num == 2) {
    messageState[oppositeFace(face)] = REVERSE1;
  }
}

void lightDisplay(byte dimness) {
  setColor(dim(WHITE, dimness));
}

void fireDisplay(byte dimness) {
  setColor(dim(RED, dimness));
  setColorOnFace(dim(WHITE, dimness), orientationFace);
  setColorOnFace(dim(WHITE, dimness), (orientationFace + 2) % 6);
  setColorOnFace(dim(WHITE, dimness), (orientationFace + 4) % 6);
}

void elecDisplay(byte dimness) {
  setColor(dim(YELLOW, dimness));
  setColorOnFace(dim(WHITE, dimness), orientationFace);
  setColorOnFace(dim(WHITE, dimness), (orientationFace + 3) % 6);
}

void grassDisplay(byte dimness) {
  setColor(dim(GREEN, dimness));
  setColorOnFace(dim(WHITE, dimness), orientationFace);
  setColorOnFace(dim(WHITE, dimness), (orientationFace + 1) % 6);
  setColorOnFace(dim(WHITE, dimness), (orientationFace + 2) % 6);
}

void waterDisplay(byte dimness) {
  setColor(dim(CYAN, dimness));
  byte spinningFace = (millis() / 700) % 6;
  setColorOnFace(dim(WHITE, dimness), spinningFace);
}

byte oppositeFace(byte face) {
  return ((face + 3) % 6);
}

byte getMessageState(byte data) {
  return (data & 7);//bits 4-5-6
}

bool getIsOn(byte data) {
  return (data >> 5); //bit 1
}
