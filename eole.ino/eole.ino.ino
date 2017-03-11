#define DEBUG

#define SWITCH 1

#define PWM_MOTOR OCR2A // digital pin (out)
#define PWM_SERVO OCR2B // digital pin (out)
#define DIR_MOTOR 12    // digital pin (out)
#define POT_MOTOR A4  // analog pin (in)

#define SERVO_DEBRAYE 0

#define DISTANCE_MIN 5
#define DISTANCE_PLATEAU 10 //Consigne pour arret lent
#define SPEED_MIN_N 0
#define SPEED_MAX_N 255
#define SPEED_MIN_R 255
#define SPEED_MAX_R 0
#define SPEED_STEP  2

char sens;
short dist;
unsigned short dist_abs, pos_mot, pos_gir;
bool flag_servo;


void calc_sens() {
// -1 si sens trigo; 1 si sens horaire; 0 si distance trop faible 
  sens = (dist_abs < DISTANCE_MIN ? 0 : ( ((dist_abs < 512) ^ (dist < 0)) ? -1 : 0) );
  
  /*
  if (dist_abs < DISTANCE_MIN)
    sens = 0;
  else if ( (dist_abs < 512) ^ (dist < 0) )
    sens = -1;
  else 
    sens = 1;}
  */
  
}

inline void calc_dist() {
  dist     = pos_mot - pos_gir;
  dist_abs = abs(dist);
}


void control_motor_normal() {
  
#ifdef DEBUG
  Serial.print("**Début** pos_gir: "); 
  Serial.print(pos_gir);
  Serial.print("pos_mot: "); 
  Serial.println(pos_mot);
#endif

  // démarrage lent
  digitalWrite(DIR_MOTOR,LOW);
  for (unsigned char speed = SPEED_MIN_N; speed < SPEED_MAX_N; speed+=SPEED_STEP){
    PWM_MOTOR = speed;
    delay(2);
  }
  pos_mot = analogRead(POT_MOTOR);
  calc_dist();

#ifdef DEBUG
  Serial.print("**Montée** pos_gir: "); 
  Serial.print(pos_gir);
  Serial.print("pos_mot: "); 
  Serial.println(pos_mot);
#endif

  // plateau
  while(dist_abs < DISTANCE_PLATEAU)
  {
    PWM_MOTOR = SPEED_MAX_N;
    delay(10);
    pos_mot = analogRead(POT_MOTOR);
    calc_dist();
  }

  // arret lent
  for (unsigned char speed = SPEED_MAX_N; speed >= SPEED_MIN_N; speed-=SPEED_STEP){
    PWM_MOTOR = speed;
    delay(2);
  }
  PWM_MOTOR = SPEED_MIN_N;

#ifdef DEBUG
  pos_mot = analogRead(POT_MOTOR);
  Serial.print("**Fin** pos_gir: "); 
  Serial.print(pos_gir);
  Serial.print("pos_mot: "); 
  Serial.println(pos_mot);
#endif
}


void control_motor_reverse() {
  
#ifdef DEBUG
  Serial.print("**Début** pos_gir: "); 
  Serial.print(pos_gir);
  Serial.print("pos_mot: "); 
  Serial.println(pos_mot);
#endif

  // démarrage lent
  digitalWrite(DIR_MOTOR,HIGH);
  for (unsigned char speed = SPEED_MIN_R; speed >=SPEED_MAX_R; speed-=SPEED_STEP){
    PWM_MOTOR = speed;
    delay(2);
  }
  pos_mot = analogRead(POT_MOTOR);
  calc_dist();

#ifdef DEBUG
  Serial.print("**Montée** pos_gir: "); 
  Serial.print(pos_gir);
  Serial.print("pos_mot: "); 
  Serial.println(pos_mot);
#endif

  // plateau
  while(dist_abs < DISTANCE_PLATEAU)
  {
    PWM_MOTOR = SPEED_MAX_R;
    delay(10);
    PWM_MOTOR = analogRead(POT_MOTOR);
    calc_dist();
  }

  // arret lent
  for (unsigned char speed = SPEED_MAX_R; speed < SPEED_MIN_R; speed+=SPEED_STEP){
    PWM_MOTOR = speed;
    delay(2);
  }
  PWM_MOTOR = SPEED_MIN_R;

#ifdef DEBUG
  pos_mot = analogRead(POT_MOTOR);
  Serial.print("**Fin** pos_gir: "); 
  Serial.print(pos_gir);
  Serial.print("pos_mot: "); 
  Serial.println(pos_mot);
#endif
}

void setup() {

  //Outputs => set 0 to motor 

  //serial

  Serial.begin(9600);
  while (!Serial);
#ifdef DEBUG
  Serial.println ("DEBUG EOLE");
#endif

}

void loop() {

  if (digitalRead(SWITCH))
  {
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


