/*

ROS Package: joy_base_control
File: teleop_twist_joy.cpp
Author: Rodrigo Castro Ochoa, rcastro@uma.es, University of Málaga.

Description:
Teleoperation of a omnidirectional mobile plattform.
This node subscribes to the topic /Joy and publishes a Twist message in the topic /cmd_base_vel.

Credits:
This code is based on the teleop_twist_joy project, available at: http://wiki.ros.org/teleop_twist_joy

*/

#include "geometry_msgs/Twist.h"
#include "ros/ros.h"
#include "sensor_msgs/Joy.h"
#include "teleop_twist_joy/teleop_twist_joy.h"

#include <map>
#include <string>

namespace teleop_twist_joy
{

  /**
   * Internal members of class. This is the pimpl idiom, and allows more flexibility in adding
   * parameters later without breaking ABI compatibility, for robots which link TeleopTwistJoy
   * directly into base nodes.
   */
  struct TeleopTwistJoy::Impl
  {
    // Members fuctions
    void printTwistInfo(const geometry_msgs::Twist &velocity, const std::string &info_string);
    void joyCallback(const sensor_msgs::Joy::ConstPtr &joy);
    void ModifyVelocity(const sensor_msgs::Joy::ConstPtr &joy_msg, float &scale, float &max_vel);

    // ROS subscribers and publisher
    ros::Subscriber joy_sub;
    ros::Publisher cmd_vel_pub;

    geometry_msgs::Twist cmd_vel_msg;

    int enable_mov;
    int increment_vel;
    int decrement_vel;
    float mov_vel;
    float orientation_vel;
    float min_vel = 0.1;
    float max_vel;

    float reaction_t = 0.5; // Tiempo en segundo de reaccion del operador

    std::map<std::string, int> axis_position_map;
    std::map<std::string, int> axis_orientation_map;
  };

  /**
   * Constructs TeleopTwistJoy.
   * \param nh NodeHandle to use for setting up the publisher and subscriber.
   * \param nh_param NodeHandle to use for searching for configuration parameters.
   */
  TeleopTwistJoy::TeleopTwistJoy(ros::NodeHandle *nh, ros::NodeHandle *nh_param)
  {
    pimpl_ = new Impl;

    pimpl_->cmd_vel_pub = nh->advertise<geometry_msgs::Twist>("cmd_base_vel", 1, true);
    pimpl_->joy_sub = nh->subscribe<sensor_msgs::Joy>("joy", 1, &TeleopTwistJoy::Impl::joyCallback, pimpl_);

    // Asignacion de botones
    nh_param->param<int>("enable_mov", pimpl_->enable_mov, 0);
    nh_param->param<int>("increment_velocity", pimpl_->increment_vel, -1);
    nh_param->param<int>("decrement_velocity", pimpl_->decrement_vel, -1);

    // Asignacion de ejes
    nh_param->getParam("axis_position_map", pimpl_->axis_position_map);
    nh_param->getParam("axis_orientation_map", pimpl_->axis_orientation_map);
    nh_param->getParam("max_displacement_in_a_second", pimpl_->max_vel);

    // Valores iniciales
    pimpl_->mov_vel = 0.5;

  }

  void TeleopTwistJoy::Impl::printTwistInfo(const geometry_msgs::Twist &velocity, const std::string &info_string)
  {
    ROS_INFO("%s - Lineal (x, y): (%.5f, %.5f), Angular (z): (%.5f)",
             info_string.c_str(),
             velocity.linear.x, velocity.linear.y,
             velocity.angular.z);
  }

  double getVal(const sensor_msgs::Joy::ConstPtr &joy_msg, const std::map<std::string, int> &axis_map, const std::string &fieldname)
  {
    /*
    Función que obtiene valores especificos del mensaje del joystick:
      - joy_msg: mensaje joy del cual se va a obtener la informacion
      - axis_map: mapa de ejes de control
      - fieldname: campo que se quiere obtener [x,y,z] o [x,y,z,w]
    */

    if (axis_map.find(fieldname) == axis_map.end() || joy_msg->axes.size() <= axis_map.at(fieldname))
    {
      return 0.0;
    }

    return joy_msg->axes[axis_map.at(fieldname)];
  }

  void TeleopTwistJoy::Impl::ModifyVelocity(const sensor_msgs::Joy::ConstPtr &joy_msg,
                                            float &scale,
                                            float &max_vel)
  {
    // Modifica la velocidad para una escala determinada 
    if (joy_msg->buttons[increment_vel])
    {
      scale = std::min(static_cast<double>(scale * 1.2), static_cast<double>(max_vel));
      ROS_INFO("Escala de velocidad incrementada a %f", scale);
    }
    else if (joy_msg->buttons[decrement_vel])
    {
      scale = std::max(static_cast<double>(scale / 1.2), static_cast<double>(min_vel));
      ROS_INFO("Escala de velocidad decrementada a %f", scale);
    }
    ros::Duration(reaction_t).sleep(); // Espera un tiempo de reaccion
  }

  void TeleopTwistJoy::Impl::joyCallback(const sensor_msgs::Joy::ConstPtr &joy_msg)
  {
    // Función que contiene el bucle de teleoperacion
    if (joy_msg->buttons[enable_mov])
    {
      ROS_INFO("Boton B pulsado");

      if (joy_msg->buttons[increment_vel] || joy_msg->buttons[decrement_vel])
      {
        // Modificar velocidad
        ModifyVelocity(joy_msg, mov_vel, max_vel);
      }
      else
      {
        // Comandar velocidad segun los valores de los ejes del mando
        ROS_INFO("Boton B pulsado");
        cmd_vel_msg.linear.x = mov_vel * getVal(joy_msg, axis_position_map, "x");
        cmd_vel_msg.linear.y = mov_vel * getVal(joy_msg, axis_position_map, "y");
        cmd_vel_msg.angular.z = mov_vel * getVal(joy_msg, axis_orientation_map, "z");
      }
    }
    else
    { // Si no se pulsa B -> Decelera
      cmd_vel_msg.linear.x = 0.0;
      cmd_vel_msg.linear.y = 0.0;
      cmd_vel_msg.angular.z = 0.0;
    }
    // Publicar e imprimir velocidad publicada
    cmd_vel_pub.publish(cmd_vel_msg);
    printTwistInfo(cmd_vel_msg, "Velocidad publicada");
  }

} // namespace teleop_twist_joy
