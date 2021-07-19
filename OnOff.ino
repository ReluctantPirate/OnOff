bool isOn = false;
byte orientationFace = 0;

enum messages {INERT, REVERSE1, LISTENING, RESET};
byte messageState[6] = {INERT, INERT, INERT, INERT, INERT, INERT};
bool activelyMessaging = false;

void setup() {

}

void loop() {
  //listen for clicks
  if (buttonSingleClicked()) {
    FOREACH_FACE(f) {
      messageState[f] = INERT;
    }

    sendLightSignal();
    isOn = !isOn;
    //orientationFace = (orientationFace + 1) % 6;
  }

  if (buttonDoubleClicked()) {

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
  lightDisplay();

}

void sendLightSignal() {//reverse all neigbors
  FOREACH_FACE(f) {
    messageState[f] = REVERSE1;
  }
}

void inertLoop(byte face) {
  if (!isValueReceivedOnFaceExpired(face)) {//there is something at this face
    byte neighborData = getLastValueReceivedOnFace(face);
    if (getMessageState(neighborData) != INERT && getMessageState(neighborData) != LISTENING) {//this face is actively communicating something to me
      messageState[face] = LISTENING;
      doReverse(1, face);
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
}

void lightDisplay(byte dimness) {
  setColor(dim(WHITE, dimness));
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
