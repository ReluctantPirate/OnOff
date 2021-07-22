/*
   On-Off Clouds Game

   Basic shared game state for play, win, and reset
*/

enum signalStates {INERT, GO, RESOLVE};
byte signalState = INERT;

enum gameModes {PLAY, WIN, RESET};//these modes will simply be different colors
byte gameMode = PLAY;//the default mode when the game begins

bool isOn = false;

enum pressStates {NONE, PRESS, ACK};
byte myPressStates[6] = {NONE, NONE, NONE, NONE, NONE, NONE};
byte wasPressed = false;

Timer resetTimer;

void setup() {

}

void loop() {

  // The following listens for and updates game state across all Blinks
  switch (signalState) {
    case INERT:
      inertLoop();
      break;
    case GO:
      goLoop();
      break;
    case RESOLVE:
      resolveLoop();
      break;
  }

  // The following is loops for each of our game states
  switch (gameMode) {
    case PLAY:
      playLoop();
      break;
    case WIN:
      winLoop();
      break;
    case RESET:
      resetLoop();
      break;
  }

  // communicate with neighbors
  // share both signalState (i.e. when to change) and the game mode
  FOREACH_FACE(f) {
    byte sendData = (myPressStates[f] << 4) + (signalState << 2) + (gameMode);
    setValueSentOnFace(sendData, f);
  }
}

/*
   Play Loop
*/
void playLoop() {

  // press to start
  if (buttonDoubleClicked()) {
    changeMode(RESET);  // change game mode on all Blinks
  }

  if (buttonSingleClicked()) {
    // communicate to each neighbor to change
    FOREACH_FACE(f) {
      myPressStates[f] = PRESS;
    }
    // toggle on/off
    isOn = !isOn;

    wasPressed = true;
  }

  // communicate with neighbors
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      byte neighborPressState = getPressState(getLastValueReceivedOnFace(f));
      if (myPressStates[f] == NONE) {
        if (neighborPressState == PRESS) {
          myPressStates[f] = ACK;
        }
      }
      else if (myPressStates[f] == PRESS) {
        if (neighborPressState == ACK) {
          myPressStates[f] = ACK;
        }
      }
      else if (myPressStates[f] == ACK && !wasPressed) {
        if (neighborPressState != PRESS) {
          myPressStates[f] = NONE;
          isOn = !isOn;
        }
      }
    }
  }

  // check that all acknowledged and then return to none
  if (wasPressed) {
    bool allNeighborsReceivedPress = true;
    FOREACH_FACE(f) {
      if (!isValueReceivedOnFaceExpired(f)) {
        if (myPressStates[f] == ACK) {
          continue;
        }
        else {
          allNeighborsReceivedPress = false;
        }
      }
    }
    if (allNeighborsReceivedPress) {
      FOREACH_FACE(f) {
        myPressStates[f] = NONE;
        wasPressed = false;
      }
    }
  }

  if (isOn) {
    setColor(WHITE);
  }
  else {
    setColor(OFF);
    setColorOnFace(dim(BLUE, 128), 0);
    setColorOnFace(dim(BLUE, 128), 2);
    setColorOnFace(dim(BLUE, 128), 4);
  }

  //  // Debug by showing comm_states
  //  FOREACH_FACE(f) {
  //    switch(myPressStates[f]) {
  //      case NONE:
  //        setColorOnFace(WHITE, f);
  //        break;
  //      case PRESS:
  //        setColorOnFace(RED, f);
  //        break;
  //      case ACK:
  //        setColorOnFace(YELLOW, f);
  //        break;
  //    }
  //  }
}


/*
   Win Loop
   do this when the board has determined it is a win
*/
void winLoop() {

}


/*
   Reset Loop
*/
void resetLoop() {

  isOn = false; // return to off
  FOREACH_FACE(f) {
    myPressStates[f] = NONE;
  }

  setColor(dim(WHITE, resetTimer.getRemaining()));
  // TODO: maybe do a 1 second animation to show that a wipe has happened then return to play..
  if (resetTimer.isExpired()) {
    changeMode(PLAY);
  }
}

/*
   pass this a game mode to switch to
*/
void changeMode( byte mode ) {
  gameMode = mode;  // change my own mode
  signalState = GO; // signal my neighbors

  // handle any items that a game should do once when it changes
  if (gameMode == PLAY) {
    // nothing at the moment
  }
  else if (gameMode == WIN) {
    // nothing at the moment
  }
  else if (gameMode == RESET) {
    resetTimer.set(255);
  }
}


/*
   This loop looks for a GO signalState
   Also gets the new gameMode
*/
void inertLoop() {

  //listen for neighbors in GO
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//a neighbor!
      if (getSignalState(getLastValueReceivedOnFace(f)) == GO) {//a neighbor saying GO!
        byte neighborGameMode = getGameMode(getLastValueReceivedOnFace(f));
        changeMode(neighborGameMode);
      }
    }
  }
}

/*
   If all of my neighbors are in GO or RESOLVE, then I can RESOLVE
*/
void goLoop() {
  signalState = RESOLVE;//I default to this at the start of the loop. Only if I see a problem does this not happen

  //look for neighbors who have not heard the GO news
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//a neighbor!
      if (getSignalState(getLastValueReceivedOnFace(f)) == INERT) {//This neighbor doesn't know it's GO time. Stay in GO
        signalState = GO;
      }
    }
  }
}

/*
   This loop returns me to inert once everyone around me has RESOLVED
   Now receive the game mode
*/
void resolveLoop() {
  signalState = INERT;//I default to this at the start of the loop. Only if I see a problem does this not happen

  //look for neighbors who have not moved to RESOLVE
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {//a neighbor!
      if (getSignalState(getLastValueReceivedOnFace(f)) == GO) {//This neighbor isn't in RESOLVE. Stay in RESOLVE
        signalState = RESOLVE;
      }
    }
  }
}


byte getGameMode(byte data) {
  return (data & 3);//returns bits E and F
}

byte getSignalState(byte data) {
  return ((data >> 2) & 3);//returns bits C and D
}

byte getPressState(byte data) {
  return ((data >> 4) & 3);//returns bit A and B
}
