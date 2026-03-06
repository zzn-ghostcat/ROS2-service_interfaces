#include "rclcpp/rclcpp.hpp"
#include "chapt4_interfaces/srv/partol.hpp"
#include <chrono>
#include <ctime>
#include "rcl_interfaces/msg/parameter.hpp"
#include "rcl_interfaces/msg/parameter_value.hpp"
#include "rcl_interfaces/msg/parameter_type.hpp"
#include "rcl_interfaces/srv/set_parameters.hpp"

using SetP = rcl_interfaces::srv::SetParameters;
using Partol = chapt4_interfaces::srv::Partol;
using namespace std::chrono_literals;

class PartolClient : public rclcpp::Node
{
private:
    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::Client<Partol>::SharedPtr partol_client_;
public:
    explicit PartolClient() : Node("turtle_control")
    {   this->declare_parameter<double>("k", 0.5);  // 声明参数 'k'
        srand(time(NULL));//初始化随机数种子
        partol_client_ = this->create_client<Partol>("partol");
        timer_ = this->create_wall_timer(10s,[&]()->void{
            //检测服务是否上线
            while (!this->partol_client_->wait_for_service(1s))
            {
                if(!rclcpp::ok()){
                    RCLCPP_ERROR(this->get_logger(),"等待服务上线过程中RCLCPP挂了");
                    return;
                }
                RCLCPP_INFO(this->get_logger(),"等待服务上线过程中.....");
            }
            //构造请求对象
            auto request = std::make_shared<Partol::Request>();
            request->target_x = rand() % 15;
            request->target_y = rand() % 15;
            RCLCPP_INFO(this->get_logger(),"准备好目标点(%f,%f)", request->target_x, request->target_y);
            //发送请求
            this->partol_client_->async_send_request(request, 
                [&](rclcpp::Client<Partol>::SharedFuture result_future)->void{
                    auto response = result_future.get();
                    if(response->result == Partol::Response::SUCCESS){
                        RCLCPP_INFO(this->get_logger(),"请求服务成功");
                    }
                    if(response->result == Partol::Response::FAIL){
                        RCLCPP_INFO(this->get_logger(),"请求服务失败");
                    }
            });
        });
    }

    //创建客户端发送请求，返回结果
    SetP::Response::SharedPtr call_set_parameters(const rcl_interfaces::msg::Parameter &param)
    {
        auto param_client = this->create_client<SetP>("/turtle_control/set_parameters");
        //检测服务是否上线
        while (!param_client->wait_for_service(1s))
        {
            if(!rclcpp::ok()){
                RCLCPP_ERROR(this->get_logger(),"等待服务上线过程中RCLCPP挂了");
                return nullptr;
            }
            RCLCPP_INFO(this->get_logger(),"等待服务上线过程中.....");
        }
        //构造请求对象
        auto request = std::make_shared<SetP::Request>();
        request->parameters.push_back(param);
        
        // RCLCPP_INFO(this->get_logger(),"准备好目标点(%f,%f)", request->target_x, request->target_y);
        //发送请求
        auto future = param_client->async_send_request(request);
        rclcpp::spin_until_future_complete(this->get_node_base_interface(), future);
        auto response = future.get();
        return response;
    }
    //更新参数K
    void update_server_param_k(double k)
    {
        //创建参数对象
        auto param = rcl_interfaces::msg::Parameter();
        param.name = "k";
        //创建参数值
        auto param_value = rcl_interfaces::msg::ParameterValue();
        param_value.type = rcl_interfaces::msg::ParameterType::PARAMETER_DOUBLE;
        param_value.double_value = k;
        param.value = param_value;
        //更新参数
        auto response = this->call_set_parameters(param);
        if(response == NULL)
        {
            RCLCPP_INFO(this->get_logger(), "参数更新失败");
            return;
        }
        for(auto res : response->results)
        {
            if(res.successful == false)
            {
                RCLCPP_INFO(this->get_logger(), "参数更新失败,原因:%s", res.reason.c_str());
            }else{
                RCLCPP_INFO(this->get_logger(), "参数更新成功");
            }
        }
    }
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<PartolClient>();
    node->update_server_param_k(16.8);
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
