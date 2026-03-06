#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "turtlesim/msg/pose.hpp"
#include "chapt4_interfaces/srv/partol.hpp"
#include "rcl_interfaces/msg/set_parameters_result.hpp"
using Partol = chapt4_interfaces::srv::Partol;
using SetParameterResult = rcl_interfaces::msg::SetParametersResult;

class TurtleControlNode : public rclcpp::Node
{
private:
    OnSetParametersCallbackHandle::SharedPtr parameter_callback_handle_;
    rclcpp::Service<Partol>::SharedPtr partol_service;
    rclcpp::Subscription<turtlesim::msg::Pose>::SharedPtr subscriber_;
    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr publisher_;
    double target_x_{1.0};
    double target_y_{1.0};
    double k_{1.0};
    double max_speed_{1.0};

public:
    explicit TurtleControlNode(const std::string& node_name) : Node(node_name)
    {
        this->declare_parameter("k", 1.0);
        this->declare_parameter("max_speed", 1.0);
        this->get_parameter("k", k_);
        this->get_parameter("max_speed", max_speed_);
        this->set_parameter(rclcpp::Parameter("k", 2.5));
        parameter_callback_handle_ = this->add_on_set_parameters_callback([&](const std::vector<rclcpp::Parameter> & parameters)->SetParameterResult{
            SetParameterResult result;
            result.successful = true;
            for (const auto & parameter : parameters) {
                RCLCPP_INFO(this->get_logger(), "更新参数%s=%f", parameter.get_name().c_str(), parameter.as_double());
                if (parameter.get_name() == "k") {
                    k_ = parameter.as_double();
                }
                if(parameter.get_name() == "max_speed"){
                    max_speed_ = parameter.as_double();
                }
            }
            return result;
        });
        partol_service = this->create_service<Partol>("partol", [&](const Partol::Request::SharedPtr request
            , Partol::Response::SharedPtr response)->void{
                if(request->target_x > 0 && request->target_x < 12
                && request->target_y > 0 && request->target_y <12)
                {
                    this->target_x_ = request->target_x;
                    this->target_y_ = request->target_y;
                    response->result = Partol::Response::SUCCESS;
                }
                else{
                    response->result = Partol::Response::FAIL;
                }
                //额外添加参数更新回调函数，当收到客户端setParam请求时自动调用该回调函数处理

        });
        publisher_ = this->create_publisher<geometry_msgs::msg::Twist>("/turtle1/cmd_vel", 10);
        subscriber_ = this->create_subscription<turtlesim::msg::Pose>("/turtle1/pose", 10, 
            std::bind(&TurtleControlNode::on_pose_recived, this, std::placeholders::_1));
        
    }
    void on_pose_recived(const turtlesim::msg::Pose::SharedPtr pose)
    {
        auto current_x = pose->x;
        auto current_y = pose->y;
        RCLCPP_INFO(get_logger(), "CURRENT: x = %f, y = %f", current_x,current_y);

        auto distance = std::sqrt((target_x_ - current_x) * (target_x_ - current_x) 
        + (target_y_ - current_y) * (target_y_ - current_y));
        auto angle = std::atan2(target_y_ - current_y, target_x_ - current_x) - pose->theta;

        auto msg = geometry_msgs::msg::Twist();
        if(distance > 0.1)
        {
            if(fabs(angle) > 0.2){
                msg.angular.z = fabs(angle);
            }
            else{
                msg.linear.x = distance * k_;
            }
        }

        if(msg.linear.x > max_speed_){
            msg.linear.x = max_speed_;
        }
        publisher_->publish(msg);
    }
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<TurtleControlNode>("turtle_control");
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
