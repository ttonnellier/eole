#include <TimerOne.h>

#define DEBUG

#define PWM_MOTOR OCR2A 
#define PIN_P_MOT 11    // digital pin (out) : polulu pwm
#define PWM_SERVO OCR2B 
#define PIN_P_SER 3     // digital pin (out)
#define DIR_MOTOR 12    // digital pin (out) : polulu dir

#define PIN_SWITCH 1    // digital pin (in) switch
#define PIN_ANEMO  2    // digital pin (in) anemometer

#define POLULU_HIGH 13
#define POLULU_LOW  10

#define POT_MOTOR A4    // analog pin (in)
#define POT_GIROU A5    // analog pin (in)

#define SERVO_DEBRAYE   0
#define SERVO_ENCLANCHE 255

#define DISTANCE_MIN 5
#define DISTANCE_PLATEAU 10 //Consigne pour arret lent
#define SPEED_MIN_N 0
#define SPEED_MAX_N 255
#define SPEED_MIN_R 255
#define SPEED_MAX_R 0
#define SPEED_STEP  2
#define OFFSET_MOTOR 0
#define OFFSET_GIROU 0
#define ROT_MAX 50
#define DEBOUNCE_MS 10 

char sens;
short dist;
unsigned short dist_abs, pos_mot, pos_gir;

volatile bool flag_servo;
volatile unsigned int rotation;
volatile unsigned int timer_count; 
volatile unsigned long debounce;

// interruption Timer1
void isr_timer() {
    timer_count++; //toutes les 0.5s
    if(timer_count == 10) // Toutes les 5s
    {
#ifdef DEBUG    
        Serial.print ("rotation value: "); Serial.println(rotation);
#endif
        if (rotation > ROT_MAX)
            flag_servo = true;
        rotation    = 0;
        timer_count = 0;
    }
}

// interruption rotation anemo
void isr_rotation() {
    if((millis() - debounce) > DEBOUNCE_MS ) { // debounce the switch contact.
        rotation++;
        debounce = millis();
    }
}


inline void calc_sens() {
// -1 si sens trigo; 1 si sens horaire; 0 si distance trop faible 
    sens = (dist_abs < DISTANCE_MIN ? 0 : ( ((dist_abs < 512) ^ (dist < 0)) ? -1 : 1) ); 
}

inline void calc_dist() {
    dist     = pos_mot - pos_gir;
    dist_abs = abs(dist);
}

void control_motor_normal() {
  
#ifdef DEBUG
    Serial.print("**Début Norm** pos_gir: "); Serial.print(pos_gir); Serial.print("pos_mot: "); Serial.println(pos_mot);
#endif

    // démarrage lent
    digitalWrite(DIR_MOTOR,LOW);
    for (unsigned char speed = SPEED_MIN_N; speed < SPEED_MAX_N; speed+=SPEED_STEP)
    {
        PWM_MOTOR = speed;
        delay(2);
    }
    pos_mot = (analogRead(POT_MOTOR)+OFFSET_MOTOR)&1023;
    calc_dist();

#ifdef DEBUG
    Serial.print("**Montée** pos_gir: "); Serial.print(pos_gir); Serial.print("pos_mot: "); Serial.println(pos_mot);
#endif

    // plateau
    while(dist_abs < DISTANCE_PLATEAU)
    {
        PWM_MOTOR = SPEED_MAX_N;
        delay(10);
        pos_mot = (analogRead(POT_MOTOR)+OFFSET_MOTOR)&1023;
        calc_dist();
    }

    // arret lent
    for (unsigned char speed = SPEED_MAX_N; speed >= SPEED_MIN_N; speed-=SPEED_STEP)
    {
        PWM_MOTOR = speed;
        delay(2);
    }
    PWM_MOTOR = SPEED_MIN_N;

#ifdef DEBUG
    pos_mot = (analogRead(POT_MOTOR)+OFFSET_MOTOR)&1023;
    Serial.print("**Fin** pos_gir: "); Serial.print(pos_gir); Serial.print("pos_mot: "); Serial.println(pos_mot);
#endif
}


void control_motor_reverse() {
  
#ifdef DEBUG
    Serial.print("**Début rev** pos_gir: "); Serial.print(pos_gir); Serial.print("pos_mot: "); Serial.println(pos_mot);
#endif

  // démarrage lent
    digitalWrite(DIR_MOTOR,HIGH);
    for (unsigned char speed = SPEED_MIN_R; speed >=SPEED_MAX_R; speed-=SPEED_STEP)
    {
        PWM_MOTOR = speed;
        delay(2);
    }
    pos_mot = (analogRead(POT_MOTOR)+OFFSET_MOTOR)&1023;
    calc_dist();

#ifdef DEBUG
    Serial.print("**Montée** pos_gir: "); Serial.print(pos_gir);  Serial.print("pos_mot: ");   Serial.println(pos_mot);
#endif

    // plateau
    while(dist_abs < DISTANCE_PLATEAU)
    {
        PWM_MOTOR = SPEED_MAX_R;
        delay(10);
        PWM_MOTOR = (analogRead(POT_MOTOR)+OFFSET_MOTOR)&1023;
        calc_dist();
    }

    // arret lent
    for (unsigned char speed = SPEED_MAX_R; speed < SPEED_MIN_R; speed+=SPEED_STEP)
    {
        PWM_MOTOR = speed;
        delay(2);
    }
    PWM_MOTOR = SPEED_MIN_R;

#ifdef DEBUG
    pos_mot = (analogRead(POT_MOTOR)+OFFSET_MOTOR)&1023; 
    Serial.print("**Fin** pos_gir: "); Serial.print(pos_gir); Serial.print("pos_mot: "); Serial.println(pos_mot);
#endif
}

void setup() {

    // 4 fils sur polulu, une sur servo, tout en sortie
    pinMode(PIN_P_MOT  , OUTPUT);
    pinMode(DIR_MOTOR  , OUTPUT);
    pinMode(POLULU_LOW , OUTPUT);
    pinMode(POLULU_HIGH, OUTPUT);
    pinMode(PIN_P_SER  , OUTPUT);

    // Switch on bloquant
    pinMode(PIN_SWITCH , INPUT );
    // pas de config pour potard car analog 

    // init Polulu
    digitalWrite(DIR_MOTOR  ,LOW );
    digitalWrite(POLULU_LOW ,LOW );
    digitalWrite(POLULU_HIGH,HIGH);
    PWM_MOTOR = SPEED_MIN_N;
    TCCR2A    = 0b10100011; //f pwm= 7kHz
    TCCR2B    = 0b00000010;
    PWM_MOTOR = SPEED_MIN_N;

    // init servo
    PWM_SERVO  = SERVO_ENCLANCHE;
    flag_servo = false;

    //init anemo
    rotation    = 0;
    debounce    = 0;
    timer_count = 0;
    pinMode(PIN_ANEMO, INPUT);
    attachInterrupt(digitalPinToInterrupt(PIN_ANEMO), isr_rotation, FALLING);

    Timer1.initialize(500000); //0.5s
    Timer1.attachInterrupt(isr_timer); 

#ifdef DEBUG    
    Serial.begin(9600);
    while (!Serial);
    Serial.println ("DEBUG EOLE");
#endif

}

void loop() {

    if (digitalRead(PIN_SWITCH))
    {
        pos_mot = (analogRead(POT_MOTOR)+OFFSET_MOTOR)&1023;
        pos_gir = (analogRead(POT_GIROU)+OFFSET_GIROU)&1023;
        calc_dist();
        calc_sens();
    
        if (sens == -1)
            control_motor_reverse();
        else if (sens == 1)   
            control_motor_normal();

        if(flag_servo)
            PWM_SERVO=SERVO_DEBRAYE;
    }

}




