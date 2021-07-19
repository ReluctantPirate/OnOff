bool isOn = false;
byte orientationFace = 0;

enum messages {INERT, REVERSE, LISTENING, RESET};
byte messageState[6] = {INERT, INERT, INERT, INERT, INERT, INERT};
bool activelyMessaging = false;

// SYNCHRONIZED WAVES
Timer syncTimer;
#define PERIOD_DURATION 3000
#define PERIOD_DURATION_ALT 2000
#define BUFFER_DURATION 200
byte neighborState[6];
byte syncVal = 0;

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
  }

  if (buttonDoubleClicked()) {
    //this is where we send a big reset signal
  }

  //make sure the clouds are in sync
  syncLoop();

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
    byte sendData = (isOn << 5) | (messageState[f] << 1) | (syncVal);
    setValueSentOnFace(sendData, f);
  }

  //do display
  lightDisplay();

}

void sendLightSignal() {//reverse all neigbors
  FOREACH_FACE(f) {
    messageState[f] = REVERSE;
  }
}

void inertLoop(byte face) {
  if (!isValueReceivedOnFaceExpired(face)) {//there is something at this face
    byte neighborData = getLastValueReceivedOnFace(face);
    if (getMessageState(neighborData) != INERT && getMessageState(neighborData) != LISTENING) {//this face is actively communicating something to me
      messageState[face] = LISTENING;
      isOn = !isOn;
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

void lightDisplay() {
  if (isOn) {
    cloudDisplay();
  } else {
    setColor(dim(BLUE, 150));
  }
}

byte getMessageState(byte data) {
  return ((data >> 1) & 7);//bits 4-5
}

bool getIsOn(byte data) {
  return (data >> 5); //bit 1
}

bool getSyncVal(byte data) {
  return (data & 1);//returns bit 6
}

void syncLoop() {

  bool didNeighborChange = false;

  // look at our neighbors to determine if one of them passed go (changed value)
  // note: absent neighbors changing to not absent don't count
  FOREACH_FACE(f) {
    if (isValueReceivedOnFaceExpired(f)) {
      neighborState[f] = 2; // this is an absent neighbor
    }
    else {
      byte data = getLastValueReceivedOnFace(f);
      if (neighborState[f] != 2) {  // wasn't absent
        if (getSyncVal(data) != neighborState[f]) { // passed go (changed value)
          didNeighborChange = true;
        }
      }

      neighborState[f] = getSyncVal(data);  // update our record of state now that we've check it
    }
  }

  // if our neighbor passed go and we haven't done so within the buffer period, catch up and pass go as well
  // if we are due to pass go, i.e. timer expired, do so
  if ( (didNeighborChange && syncTimer.getRemaining() < PERIOD_DURATION - BUFFER_DURATION)
       || syncTimer.isExpired()
     ) {

    if (random(20) == 0) {
      syncTimer.set(PERIOD_DURATION_ALT); // aim to pass go in the defined duration
    } else {
      syncTimer.set(PERIOD_DURATION); // aim to pass go in the defined duration

    }
    syncVal = !syncVal; // change our value everytime we pass go
  }
}

#define CLOUD_MIN_BRIGHTNESS 200
#define CLOUD_MAX_BRIGHTNESS 255

void cloudDisplay() { //just displays the water beneath any piece with missing bits (lasers, mirrors, damaged ships)

  byte syncProgress = map(syncTimer.getRemaining(), 0, PERIOD_DURATION, 0, 255);
  byte syncProgressSin = sin8_C(syncProgress);
  byte syncProgressMapped = map(syncProgressSin, 0, 255, CLOUD_MIN_BRIGHTNESS, CLOUD_MAX_BRIGHTNESS);

  setColor(dim(WHITE, syncProgressMapped));

}
