import processing.serial.*;

Serial myPort;

// Declare GUI elements
TextField textFieldMinPosition;
TextField textFieldMaxPosition;
TextField textFieldFrequency;
Button buttonStart;
Button buttonStop;

void setup() {
  size(500, 300);
  myPort = new Serial(this, "/dev/cu.usbmodem1301", 9600);  // Use the correct serial port

  // Initialize GUI elements
  textFieldMinPosition = new TextField(50, 50, 200, 30, "Min Position (0-4095)");
  textFieldMaxPosition = new TextField(50, 100, 200, 30, "Max Position (0-4095)");
  textFieldFrequency = new TextField(50, 150, 200, 30, "Frequency (Hz, e.g., 0.5)");
  buttonStart = new Button(50, 200, 100, 50, "START");
  buttonStop = new Button(170, 200, 100, 50, "STOP");
}

void draw() {
  background(255);

  // Draw GUI elements
  textFieldMinPosition.display();
  textFieldMaxPosition.display();
  textFieldFrequency.display();
  buttonStart.display();
  buttonStop.display();

  // Check if START button is pressed
  if (buttonStart.isPressed()) {
    float minPosition = float(textFieldMinPosition.getValue());
    float maxPosition = float(textFieldMaxPosition.getValue());
    float frequency = float(textFieldFrequency.getValue());

    // Send commands to Arduino
    myPort.write("MIN:" + minPosition + "\n");
    myPort.write("MAX:" + maxPosition + "\n");
    myPort.write("FREQ:" + frequency + "\n");
    myPort.write("START\n");

    println("Sent commands: MIN = " + minPosition + ", MAX = " + maxPosition + ", FREQ = " + frequency);
  }

  // Check if STOP button is pressed
  if (buttonStop.isPressed()) {
    myPort.write("STOP\n");
    println("Oscillation stopped.");
  }
}

// TextField class
class TextField {
  int x, y, width, height;
  String label;
  String value = "";
  boolean isSelected = false;

  TextField(int x, int y, int width, int height, String label) {
    this.x = x;
    this.y = y;
    this.width = width;
    this.height = height;
    this.label = label;
  }

  void display() {
    // Draw the text field box
    fill(200);
    stroke(0);
    rect(x, y, width, height);
    fill(0);
    text(label, x, y - 10);

    // Draw the current value
    text(value, x + 5, y + 20);

    // Highlight if selected
    if (isSelected) {
      noFill();
      stroke(0, 0, 255); // Blue border
      rect(x, y, width, height);
    }
  }

  String getValue() {
    return value;
  }

  void keyPressed(char key) {
    if (isSelected) {
      if (key == BACKSPACE && value.length() > 0) {
        value = value.substring(0, value.length() - 1);
      } else if ((key >= '0' && key <= '9') || key == '.') {
        value += key;
      }
    }
  }

  boolean isMouseOver() {
    return mouseX >= x && mouseX <= x + width && mouseY >= y && mouseY <= y + height;
  }
}

// Button class
class Button {
  int x, y, width, height;
  String label;
  boolean pressed = false;

  Button(int x, int y, int width, int height, String label) {
    this.x = x;
    this.y = y;
    this.width = width;
    this.height = height;
    this.label = label;
  }

  void display() {
    fill(pressed ? color(100) : color(200));
    stroke(0);
    rect(x, y, width, height);
    fill(0);
    text(label, x + 10, y + 30);
  }

  boolean isPressed() {
    if (mousePressed && mouseX >= x && mouseX <= x + width && mouseY >= y && mouseY <= y + height) {
      pressed = true;
      return true;
    } else {
      pressed = false;
      return false;
    }
  }
}

// Handle key presses
void keyPressed() {
  textFieldMinPosition.keyPressed(key);
  textFieldMaxPosition.keyPressed(key);
  textFieldFrequency.keyPressed(key);
}

// Handle mouse clicks
void mousePressed() {
  // Check which text field is selected
  if (textFieldMinPosition.isMouseOver()) {
    textFieldMinPosition.isSelected = true;
    textFieldMaxPosition.isSelected = false;
    textFieldFrequency.isSelected = false;
  } else if (textFieldMaxPosition.isMouseOver()) {
    textFieldMaxPosition.isSelected = true;
    textFieldMinPosition.isSelected = false;
    textFieldFrequency.isSelected = false;
  } else if (textFieldFrequency.isMouseOver()) {
    textFieldFrequency.isSelected = true;
    textFieldMinPosition.isSelected = false;
    textFieldMaxPosition.isSelected = false;
  } else {
    textFieldMinPosition.isSelected = false;
    textFieldMaxPosition.isSelected = false;
    textFieldFrequency.isSelected = false;
  }
}
