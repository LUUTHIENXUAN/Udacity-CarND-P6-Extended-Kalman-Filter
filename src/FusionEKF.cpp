#include "FusionEKF.h"
#include "tools.h"
#include "Eigen/Dense"
#include <iostream>

#include <chrono>

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::vector;

// https://stackoverflow.com/questions/2940367/what-is-more-efficient-using-pow-to-square-or-just-multiply-it-with-itself
class Timer {
  public:
    explicit Timer () : from (std::chrono::high_resolution_clock::now()) { }

    void start () {
      from = std::chrono::high_resolution_clock::now();
    }

    double elapsed() const {
      return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - from).count() * 1.0e-6;
    }

  private:
    std::chrono::high_resolution_clock::time_point from;
};

/*
 * Constructor.
 */
FusionEKF::FusionEKF() {
  is_initialized_ = false;

  previous_timestamp_ = 0;

  // initializing matrices
  R_laser_ = MatrixXd(2, 2);
  R_radar_ = MatrixXd(3, 3);
  H_laser_ = MatrixXd(2, 4);
  Hj_ = MatrixXd(3, 4);

  //measurement covariance matrix - laser
  R_laser_ << 0.0225, 0,
              0     , 0.0225;

  //measurement covariance matrix - radar
  R_radar_ << 0.09, 0     , 0,
              0   , 0.0009, 0,
              0   , 0     , 0.09;

  /**
  TODO:
    * Finish initializing the FusionEKF.
    * Set the process and measurement noises
  */
  
  H_laser_ << 1, 0, 0, 0,
              0, 1, 0, 0;
              
  //float noise_ax = 9.0;
  //float noise_ay = 9.0;
  
  // Initial Transition Matrix
  ekf_.F_ = MatrixXd(4,4);
  ekf_.F_ << 1, 0, 1,  0,
             0, 1, 0,  0,
             0, 0, 1,  0,
             0, 0, 0,  1;
  
  // State Coveriance Matrix
  ekf_.P_ = MatrixXd(4,4);
  ekf_.P_ << 1, 0,    1,   0,
             0, 1,    0,   0,
             0, 0, 1000,   0,
             0, 0,    0,1000;
  


}

/**
* Destructor.
*/
FusionEKF::~FusionEKF() {}

void FusionEKF::ProcessMeasurement(const MeasurementPackage &measurement_pack) {
	
  Timer timer;
  timer.start();

  /*****************************************************************************
   *  Initialization
   ****************************************************************************/
  if (!is_initialized_) {
    /**
    TODO:
      * Initialize the state ekf_.x_ with the first measurement.
      * Create the covariance matrix.
      * Remember: you'll need to convert radar from polar to cartesian coordinates.
    */
    // first measurement
    cout << "EKF: " << endl;
    ekf_.x_ = VectorXd(4);
    ekf_.x_ << 0.001, 0.001, 0.001, 0.001;

    if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
      /**
      Convert radar from polar to cartesian coordinates and initialize state.
      */
      
      float rho     = measurement_pack.raw_measurements_[0];
      float phi     = measurement_pack.raw_measurements_[1];
      float rho_dot = measurement_pack.raw_measurements_[2];
      
      ekf_.x_ << rho * cos(phi), rho * sin(phi), 0, 0;
      
    }
    else if (measurement_pack.sensor_type_ == MeasurementPackage::LASER) {
      /**
      Initialize state.
      */
      
      ekf_.x_ << measurement_pack.raw_measurements_[0], measurement_pack.raw_measurements_[1], 0.0, 0.0;
    }
    
    previous_timestamp_ = measurement_pack.timestamp_;

    // done initializing, no need to predict or update
    is_initialized_ = true;
    return;
  }

  /*****************************************************************************
   *  Prediction
   ****************************************************************************/

  /**
   TODO:
     * Update the state transition matrix F according to the new elapsed time.
      - Time is measured in seconds.
     * Update the process noise covariance matrix.
     * Use noise_ax = 9 and noise_ay = 9 for your Q matrix.
   */
   //compute the time elapsed between the current and previous measurements
   float dt = (measurement_pack.timestamp_ - previous_timestamp_) / 1000000.0;	//dt - expressed in seconds
   previous_timestamp_ = measurement_pack.timestamp_;
   
   float dt_2 = dt * dt;
   float dt_3 = dt_2 * dt;
   float dt_4 = dt_3 * dt;
   
   //Modify the F matrix so that the time is integrated
   ekf_.F_(0, 2) = dt;
   ekf_.F_(1, 3) = dt;

   //set the process covariance matrix Q
   ekf_.Q_ = MatrixXd(4, 4);
   
   // change to 6.0 to see what happens
   float noise_ax = 9.0;
   float noise_ay = 9.0;
   
   ekf_.Q_ << (dt_4/4)*noise_ax,                 0, (dt_3/2)*noise_ax,                 0,
			                  0, (dt_4/4)*noise_ay,                 0, (dt_3/2)*noise_ay,
			  (dt_3/2)*noise_ax,                 0,     dt_2*noise_ax,                 0,
			                  0, (dt_3/2)*noise_ay,                 0,     dt_2*noise_ay;

   
   
   ekf_.Predict();

  /*****************************************************************************
   *  Update
   ****************************************************************************/

  /**
   TODO:
     * Use the sensor type to perform the update step.
     * Update the state and covariance matrices.
   */

  if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
    
    // Radar updates
    Hj_     = tools.CalculateJacobian(ekf_.x_);
    ekf_.H_ = Hj_;
    ekf_.R_ = R_radar_;
    
    ekf_.UpdateEKF(measurement_pack.raw_measurements_);
    
    
  } else {
	
    // Laser updates
    ekf_.H_ = H_laser_;
    ekf_.R_ = R_laser_;
    
    ekf_.Update(measurement_pack.raw_measurements_);
    
  }
  
  std::cout << "Processed time: " << timer.elapsed() << std::endl;

  // print the output
  cout << "x_ = " << ekf_.x_ << endl;
  cout << "P_ = " << ekf_.P_ << endl;
}