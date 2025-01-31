#include <ArduinoQueue.h>
#include <LiquidCrystal_I2C.h>
#include <SimpleStack.h>
#include <Wire.h>

#define __LONG_MIN__ -(1L << 31)

long shuntingYard(char* exp);

LiquidCrystal_I2C lcd(0x27, 16, 2);
String input = "";

const int buttonPins[11] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
const char symbols[] = { 'C', '=', 'B', '/', '*', '-', '+', ')', '(', '0', '1' };

void setup()
{
    Serial.begin(9600);
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    for (int i = 0; i < 11; i++)
        pinMode(buttonPins[i], INPUT_PULLUP);
}

void loop()
{
    if (Serial.available() > 0) {
        char pressed = Serial.read();
        if (pressed != 0)
            handleButtonPress(pressed);
    }
    for (int i = 0; i < 11; i++) {
        if (digitalRead(buttonPins[i]) == LOW) {
            char pressed = symbols[i];
            handleButtonPress(pressed);
            delay(300);
        }
    }
}

void handleButtonPress(char pressed)
{
    if (pressed == 'B') {
        if (input.length() > 0)
            input.remove(input.length() - 1);
        else
            return;
    } else if (pressed == 'C')
        input = "";
    else if (pressed == '(') {
        if (input.length() > 0)
            if (input[input.length() - 1] == '0' || input[input.length() - 1] == '1')
                input += '*';
        input += pressed;
    } else if (pressed == '=') {
        String result = processInput(input);
        printLCD("Result: ", result);
        if (result != "ERROR!" && result != "# Division by Zero!") {
            input = binary(result);
            if (input[0] == '-')
                input = '(' + input + ')';
        }
        return;
    } else
        input += pressed;
    printLCD("Input: ", BCD(input));
}

void printLCD(String input1, String input2)
{
    Serial.println(input1 + input2);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(input1);
    lcd.setCursor(0, 1);
    lcd.print(input2);
}

String BCD(String input)
{
    String result = "";
    String temp = "";
    for (char c : input) {
        if (c == '0' || c == '1')
            temp += c;
        else {
            if (temp.length() > 0) {
                result += String(strtol(temp.c_str(), NULL, 2));
                temp = "";
            }
            result += c;
        }
    }
    if (temp.length() > 0)
        result += String(strtol(temp.c_str(), NULL, 2));
    return result;
}

String binary(String input)
{
    String result = "";
    String temp = "";
    for (char c : input) {
        if (isdigit(c))
            temp += c;
        else {
            if (temp.length() > 0) {
                result += String(strtol(temp.c_str(), NULL, 10), BIN);
                temp = "";
            }
            result += c;
        }
    }
    if (temp.length() > 0)
        result += String(strtol(temp.c_str(), NULL, 10), BIN);
    return result;
}

String processInput(String input)
{
    if (!validateInput(input))
        return "ERROR!";
    String BCD_str = BCD(input);
    char inputCharArray[BCD_str.length() + 1]; // Create char array of appropriate size
    BCD_str.toCharArray(inputCharArray, BCD_str.length() + 1); // Convert String to char*
    long ans = shuntingYard(inputCharArray);
    if (ans == __LONG_MIN__)
        return "# Division by Zero!";
    return String(ans);
}

bool validateInput(String input)
{
    if (input[0] == '+' || input[0] == '-' || input[0] == '*' || input[0] == '/')
        return false;
    if (input[input.length() - 1] == '+' || input[input.length() - 1] == '-' || input[input.length() - 1] == '*' || input[input.length() - 1] == '/')
        return false;

    int openBrackets = 0;
    for (int i = 0; i < input.length(); i++) {
        char c = input[i];
        if (c == '(')
            openBrackets++;
        if (c == ')')
            openBrackets--;
        if (openBrackets < 0)
            return false;
        if (i > 0) {
            char prev = input[i - 1];
            if ((prev == '+' || prev == '-' || prev == '*' || prev == '/') && (c == '+' || c == '-' || c == '*' || c == '/'))
                return false;
            if ((prev == '0' || prev == '1' || prev == ')') && c == '(')
                return false;
            if (prev == ')' && (c == '0' || c == '1' || c == '('))
                return false;
            if ((c == '*' || c == '/') && prev == '(')
                return false;
        }
    }
    return openBrackets == 0;
}

// LOGIC
long shuntingYard(char* exp)
{
    ArduinoQueue<long> output(64);
    SimpleStack<char> operators(64);

    for (int i = 0; exp[i] != '\0'; i++) {
        if (exp[i] == '(' && (exp[i + 1] == '+' || exp[i + 1] == '-')) {
            bool sign = exp[i + 1] == '-';
            i += 2;
            long number = 0;
            do {
                number *= 10;
                number += exp[i] - '0';
            } while (exp[++i] >= '0' && exp[i] <= '9');
            if (sign)
                number *= -1;
            output.enqueue(number);
        } else if (exp[i] == '(')
            operators.push(exp[i]);
        else if (exp[i] == ')') {
            char topElement;
            while (operators.peek(&topElement) && topElement != '(') {
                char popped;
                operators.pop(&popped);
                output.enqueue(static_cast<long>(popped) + (1L << 30));
            }
            operators.pop(&topElement);
        } else if (exp[i] == '+' || exp[i] == '-') {
            char topElement;
            while (!operators.isEmpty() && operators.peek(&topElement) && topElement != '(') {
                char popped;
                operators.pop(&popped);
                output.enqueue(static_cast<long>(popped) + (1L << 30));
            }
            operators.push(exp[i]);
        } else if (exp[i] == '*' || exp[i] == '/') {
            char topElement;
            while (!operators.isEmpty() && operators.peek(&topElement) && (topElement == '*' || topElement == '/')) {
                char popped;
                operators.pop(&popped);
                output.enqueue(static_cast<long>(popped) + (1L << 30));
            }
            operators.push(exp[i]);
        } else {
            long number = 0;
            do {
                number *= 10;
                number += exp[i] - '0';
            } while (exp[++i] >= '0' && exp[i] <= '9');
            output.enqueue(number);
            i--;
        }
    }

    while (!operators.isEmpty()) {
        char popped;
        operators.pop(&popped);
        output.enqueue(static_cast<long>(popped) + (1L << 30));
    }

    SimpleStack<long> answer(64);
    while (!output.isEmpty()) {
        if (output.getHead() > (1L << 30)) {
            char op = static_cast<char>(output.getHead() - (1L << 30));
            long first_op, second_op;
            answer.pop(&first_op);
            answer.pop(&second_op);

            if (op == '+')
                answer.push(second_op + first_op);
            else if (op == '-')
                answer.push(second_op - first_op);
            else if (op == '*')
                answer.push(second_op * first_op);
            else {
                if (first_op == 0)
                    return __LONG_MIN__;
                answer.push(second_op / first_op);
            }
            output.dequeue();
        } else
            answer.push(output.dequeue());
    }

    long result;
    answer.pop(&result);
    return result;
}