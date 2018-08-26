/*********************************************************************************
 *  Copyright (c) 2015, The Underwater Robotics Team
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *********************************************************************************/

#include "ros/ros.h"
#include "message_filters/subscriber.h"
#include "message_filters/synchronizer.h"
#include "message_filters/sync_policies/approximate_time.h"
#include "tf/transform_broadcaster.h"
#include "sensor_msgs/Imu.h"
#include "imu_3dm_gx4/FilterOutput.h"

ros::Publisher unifier;
sensor_msgs::Imu state;

// tf::TransformBroadcaster bcast;
// tf::StampedTransform tform;
// tf::Quaternion qturn;

tf::Matrix3x3 rotation = tf::Matrix3x3(1, 0, 0, 0, -1, 0, 0, 0, -1);

void callback(const sensor_msgs::Imu::ConstPtr& imu, const imu_3dm_gx4::FilterOutput::ConstPtr& filter)
{
  state.header = imu->header;

  tf::Quaternion quaternion;
  tf::quaternionMsgToTF(filter->quaternion, quaternion);
  tf::Matrix3x3 orientation = tf::Matrix3x3(quaternion);
  orientation = rotation * orientation;
  double yaw, pitch, roll;
  orientation.getEulerYPR(yaw, pitch, roll);
  quaternion.setEuler(yaw, pitch, roll);
  tf::quaternionTFToMsg(quaternion, state.orientation);
  //state.orientation_covariance = filter->orientation_covariance;

  tf::Vector3 angular_velocity;
  tf::vector3MsgToTF(imu->angular_velocity, angular_velocity);
  angular_velocity = rotation * angular_velocity;
  tf::vector3TFToMsg(angular_velocity, state.angular_velocity);
  state.angular_velocity_covariance = imu->angular_velocity_covariance;

  tf::Vector3 linear_acceleration;
  tf::vector3MsgToTF(imu->linear_acceleration, linear_acceleration);
  linear_acceleration = rotation * linear_acceleration;
  tf::vector3TFToMsg(linear_acceleration, state.linear_acceleration);
  state.linear_acceleration_covariance = imu->linear_acceleration_covariance;

  unifier.publish(state);

  // tf::quaternionMsgToTF(filter->orientation, qturn);
  // tform.setRotation(qturn);
  // tform.stamp = imu->header.stamp;
  // bcast.sendTransform(tform);
}

int main(int argc, char** argv)
{
  ros::init(argc, argv, "imu_state");

  ros::NodeHandle nh;

  message_filters::Subscriber<sensor_msgs::Imu> imu_sub(nh, "imu/imu", 1);
  message_filters::Subscriber<imu_3dm_gx4::FilterOutput> filter_sub(nh, "imu/filter", 1);

  typedef message_filters::sync_policies::ApproximateTime<sensor_msgs::Imu, imu_3dm_gx4::FilterOutput> approx;
  message_filters::Synchronizer<approx> sync(approx(10), imu_sub, filter_sub);
  sync.registerCallback(boost::bind(&callback, _1, _2));

  unifier = nh.advertise<sensor_msgs::Imu>("state/imu", 1);

  // tform.getOrigin().setX(0.0);
  // tform.getOrigin().setY(0.0);
  // tform.getOrigin().setZ(0.0);
  // tform.frame_id_ = "";
  // tform.child_frame_id_ = "";

  ros::spin();
}
