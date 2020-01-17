bool isOn = false;
enum messages {INERT, CLICKED, LISTENING};
byte messageState = INERT;

byte clickedNeighbor;

void setup() {
  // put your setup code here, to run once:
  randomize();
  if (random(1) == 1) {
    isOn = true;
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  if (buttonSingleClicked()) {
    //change color, make neighbors change color
    messageState = CLICKED;
  }

  switch (messageState) {
    case INERT:
      inertLoop();
      break;
    case CLICKED:
      clickedLoop();
      break;
    case LISTENING:
      listeningLoop();
      break;
  }

  setValueSentOnAllFaces(messageState);

  if (messageState == INERT) {
    if (isOn) {
      setColor(WHITE);
    } else {
      setColor(dim(WHITE, 75));
    }
  } else if (messageState == CLICKED) {
    setColor(RED);
  } else if (messageState == LISTENING) {
    setColor(BLUE);
  }
}

void inertLoop() {
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//neighbor!
      if (getLastValueReceivedOnFace(f) == CLICKED) {
        messageState = LISTENING;
        clickedNeighbor = f;
      }
    }
  }
}

void clickedLoop() {
  byte inertNeighborsRemaining = 0;

  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//neighbor!
      if (getLastValueReceivedOnFace(f) == INERT) {
        inertNeighborsRemaining++;
      }
    }
  }

  if (inertNeighborsRemaining == 0) {
    switchOn();
    messageState = INERT;
  }
}

void listeningLoop() {
  if (!isValueReceivedOnFaceExpired(clickedNeighbor)) {//clicked neighbor still there
    if (getLastValueReceivedOnFace(clickedNeighbor) == INERT) {
      switchOn();
      messageState = INERT;
    }
  }
}

void switchOn() {
  if (isOn) {
    isOn = false;
  } else {
    isOn = true;
  }
}
