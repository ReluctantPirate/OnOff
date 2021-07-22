/*
 * On-Off Clouds Game
 * 
 * Basic shared game state for play, win, and reset
*/

enum signalStates {INERT, GO, RESOLVE};
byte signalState = INERT;

enum gameModes {PLAY, ACTION, WIN, RESET};//these modes will simply be different colors
byte gameMode = PLAY;//the default mode when the game begins

bool isOn = false;

bool myPressState = false;

Timer resetTimer;
Timer actionTimer;

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
    case ACTION:
      actionLoop();
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
  byte sendData = (myPressState << 4) + (signalState << 2) + (gameMode);
  setValueSentOnAllFaces(sendData);
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
    // communicate to neighbors to change
    myPressState = true;
    changeMode(ACTION);
  }

  if(isOn) {
    setColor(WHITE);
  }
  else {
    setColor(OFF);
    setColorOnFace(dim(BLUE,128), 0);
    setColorOnFace(dim(BLUE,128), 2);
    setColorOnFace(dim(BLUE,128), 4);
  }
}


/*
   Action Loop
   do this when a piece is switched
*/
void actionLoop() {
  // check for win condition...

  // return to play
  if(actionTimer.isExpired()) {
    myPressState = false;
    changeMode(PLAY);
  }
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

  setColor(dim(WHITE,resetTimer.getRemaining()));
  // TODO: maybe do a 1 second animation to show that a wipe has happened then return to play..
  if(resetTimer.isExpired()) {
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
  else if (gameMode == ACTION) {
    flip();
    actionTimer.set(255);
  }
  else if (gameMode == WIN) {
    // nothing at the moment
  }
  else if (gameMode == RESET) {
    resetTimer.set(255);
  }
}

void flip() {
  // flip state
  // if my neighbor is the pressed one, then I flip
  FOREACH_FACE(f) {
    if(!isValueReceivedOnFaceExpired(f)) {
      byte neighborPressState = getPressState(getLastValueReceivedOnFace(f));
      if(neighborPressState == true) {
        isOn = !isOn;
        break;
      }
    }
  }

  if(myPressState == true) {
    isOn = !isOn;
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
  return ((data >> 4) & 1);//returns bit A and B
}
