import launch
import launch_ros
from ament_index_python.packages import get_package_share_directory

def generate_launch_description():
    aciton_declare_startup_rqt = launch.actions.DeclareLaunchArgument('startup_rqt',default_value='False')
    startup_rqt = launch.substitutions.LaunchConfiguration('startup_rqt',default='False')

    # 包含其他launch
    multisim_launch_path = [get_package_share_directory('turtlesim'),'/launch/','multisim.launch.py']
    action_include_launch = launch.actions.IncludeLaunchDescription(
        launch.launch_description_sources.PythonLaunchDescriptionSource(
            multisim_launch_path
        )
    )
    #打印数据
    action_log_info = launch.actions.LogInfo(msg=str(multisim_launch_path))
    # 执行命令行
    action_topic_list = launch.actions.ExecuteProcess(
        condition=launch.conditions.IfCondition(startup_rqt),
        cmd=['rqt']
    )
    # 组织动作成组，把多个动作成组
    action_group = launch.actions.GroupAction([
        # 定时器
        launch.actions.TimerAction(period=2.0,actions=[action_include_launch]),
        launch.actions.TimerAction(period=4.0,actions=[action_topic_list])
    ])
    
    return launch.LaunchDescription([
        aciton_declare_startup_rqt,
        action_log_info,
        action_group
    ])
