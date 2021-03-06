/*-------------------------------------------------------------------------------------
Credit to Joop Brokkking for I2C tutorials and parts of this code. Check put his videos on youtube.

Summary:
This is a simple program to display the angle from a MPU-6050 IMU. It uses both the Accelerometer
and the Gyro to get an accurate reading. To start the program, upload it and open the serial monitor/serial plotter.
The gyro should start calibrating. It will then start printing the roll and pitch angles. It might not
be exactly in degrees, so depending on your setup, you will have to scale the output. This can be done
by adjusting the "degreescale" variable. 

Wiring:
Gyro - Arduino UNO
VCC  -  5V
GND  -  GND
SDA  -  A4
SCL  -  A5
----------------------------------------------------------------------------------------*/

//Include I2C library
#include <Wire.h>

//Declaring Variables-------------------------------------------------------------------
//Raw reading variables
int gyro_x, gyro_y, gyro_z;//raw gyro readings (angular vel)
long acc_x, acc_y, acc_z, acc_total_vector;//raw accelerometer readings (acceleration)
int temperature;//temperature
//Gyro variables
long gyro_x_cal, gyro_y_cal, gyro_z_cal;//gyro calibration offset variables
float angle_pitch, angle_roll;//angular position (integrated angular velocity)
boolean set_gyro_angles;//To tell whether the gyro has been started
//Accelerometer variables
float angle_roll_acc, angle_pitch_acc;//Accelerometer angles 
//Combined variables
float angle_pitch_output, angle_roll_output;//Output angles
const float acc_weight = 0.03;//Weight of accelerometer angles when calculating output angles
const float degreescale = 3.76;
//Loop settings
long loop_timer;

void setup() {
  
  //Start I2C as master
  Wire.begin();
  
  //Serial monitor
  Serial.begin(9600);
  
  //Setup the registers of the MPU-6050 (500dfs / +/-8g) and start the gyro
  setup_mpu_6050_registers();

  calibrateGyro();
  
  //Reset the loop timer
  loop_timer = micros();
  
}

void loop(){

  calculateAngles();
  
  //Display the angles
  write_Telem();
  
  while(micros() - loop_timer < 4000);//Wait until the loop_timer reaches 4000us (250Hz) before starting the next loop
  loop_timer = micros();//Reset the loop timer
}


void calculateAngles(){
  //Read the raw acc and gyro data from the MPU-6050
  read_mpu_6050_data();

  //Gyro angle calculations------------------------------------------------------------------------------------------------
  //Subtract the offset calibration values from the raw gyro values
  gyro_x -= gyro_x_cal;
  gyro_y -= gyro_y_cal;
  gyro_z -= gyro_z_cal;
  
  //Calculate the angular velocities and integrate to get angular positions
  angle_pitch += gyro_x * 0.0000611;
  angle_roll += gyro_y * 0.0000611;

  //Acount for tilt in one axis affecting the other axis
  angle_pitch += angle_roll * sin(gyro_z * 0.000001066);//If the IMU has yawed transfer the roll angle to the pitch angel
  angle_roll -= angle_pitch * sin(gyro_z * 0.000001066);//If the IMU has pitched transfer the pitch angle to the roll angel
  
  //Accelerometer angle calculations----------------------------------------------------------------------------------------
  acc_total_vector = sqrt((acc_x*acc_x)+(acc_y*acc_y)+(acc_z*acc_z));//Calculate the total accelerometer vector
  
  //Calculate the pitch and roll angles according to the accelerometer
  angle_pitch_acc = asin((float)acc_y/acc_total_vector)* 57.296;//57.296 because the Arduino asin function is in radians
  angle_roll_acc = asin((float)acc_x/acc_total_vector)* -57.296;
  
  //Place the MPU-6050 spirit level and note the values in the following two lines for calibration
  angle_pitch_acc -= 0.0;//Accelerometer calibration value for pitch
  angle_roll_acc -= 0.8;//Accelerometer calibration value for roll

  //Combine the two with a weighted average--------------------------------------------------------------------------------
  if(set_gyro_angles){
    //If the IMU is already started
    //Correct the drift of the gyro angle with the accelerometer angle
    angle_pitch = angle_pitch * (1-acc_weight) + angle_pitch_acc * acc_weight;//Adjust the acc_weight variable
    angle_roll = angle_roll * (1-acc_weight) + angle_roll_acc * acc_weight;//Adjust the acc_weight variable
  }else{
    //At first start
    //Set the gyro angles equal to the accelerometer angles
    angle_pitch = angle_pitch_acc;
    angle_roll = angle_roll_acc;
    set_gyro_angles = true;//Tell program that IMU is started
  }

  //Apply scaling or filters here
  angle_pitch_output = angle_pitch * degreescale;
  angle_roll_output = angle_roll * degreescale;
  
}

void read_mpu_6050_data(){
  //Subroutine for reading the raw gyro and accelerometer data
  Wire.beginTransmission(0x68);                                        //Start communicating with the MPU-6050
  Wire.write(0x3B);                                                    //Send the requested starting register
  Wire.endTransmission();                                              //End the transmission
  Wire.requestFrom(0x68,14);                                           //Request 14 bytes from the MPU-6050
  while(Wire.available() < 14);                                        //Wait until all the bytes are received
  acc_x = Wire.read()<<8|Wire.read();                                  //Add the low and high byte to the acc_x variable
  acc_y = Wire.read()<<8|Wire.read();                                  //Add the low and high byte to the acc_y variable
  acc_z = Wire.read()<<8|Wire.read();                                  //Add the low and high byte to the acc_z variable
  temperature = Wire.read()<<8|Wire.read();                            //Add the low and high byte to the temperature variable
  gyro_x = Wire.read()<<8|Wire.read();                                 //Add the low and high byte to the gyro_x variable
  gyro_y = Wire.read()<<8|Wire.read();                                 //Add the low and high byte to the gyro_y variable
  gyro_z = Wire.read()<<8|Wire.read();                                 //Add the low and high byte to the gyro_z variable
  
}

void write_Telem(){
  Serial.print((int)angle_pitch_output);
  Serial.print(", ");
  Serial.println((int)angle_roll_output);
}

void calibrateGyro(){
  Serial.print("  MPU-6050 IMU");
  Serial.print("     FINAL");
  Serial.print("Calibrating gyro");
  for (int cal_int = 0; cal_int < 2000 ; cal_int ++){
    if(cal_int % 125 == 0)Serial.print(".");
    read_mpu_6050_data();
    //Add the gyro offest to the variables for averaging
    gyro_x_cal += gyro_x;
    gyro_y_cal += gyro_y;
    gyro_z_cal += gyro_z;
    delay(3);//Delay 3us to simulate the 250Hz program loop
    
  }
  //Divide the calibration variables by 2000 to get the avarage offset
  gyro_x_cal /= 2000;
  gyro_y_cal /= 2000;
  gyro_z_cal /= 2000;
}

void setup_mpu_6050_registers(){
  //Activate the MPU-6050
  Wire.beginTransmission(0x68);                                        //Start communicating with the MPU-6050
  Wire.write(0x6B);                                                    //Send the requested starting register
  Wire.write(0x00);                                                    //Set the requested starting register
  Wire.endTransmission();                                              //End the transmission
  //Configure the accelerometer (+/-8g)
  Wire.beginTransmission(0x68);                                        //Start communicating with the MPU-6050
  Wire.write(0x1C);                                                    //Send the requested starting register
  Wire.write(0x10);                                                    //Set the requested starting register
  Wire.endTransmission();                                              //End the transmission
  //Configure the gyro (500dps full scale)
  Wire.beginTransmission(0x68);                                        //Start communicating with the MPU-6050
  Wire.write(0x1B);                                                    //Send the requested starting register
  Wire.write(0x08);                                                    //Set the requested starting register
  Wire.endTransmission();                                              //End the transmission
}
