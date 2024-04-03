#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sra_board.h"
#include "tuning_http_server.h"

#define MODE NORMAL_MODE
#define BLACK_MARGIN 0    // Change to 0 for black background
#define WHITE_MARGIN 4095 // Change to 4095 for white line
#define bound_LSA_LOW 0
#define bound_LSA_HIGH 1000
#define BLACK_BOUNDARY 930 // Boundary value to distinguish between black and white readings

/*
 * weights given to respective line sensor
 */
const int weights[5] = {-5, -3, 1, 3, 5};

/*
 * Motor value bounds
 */
int optimum_duty_cycle = 57;
int lower_duty_cycle = 45;
int higher_duty_cycle = 65;
float left_duty_cycle = 0, right_duty_cycle = 0;

/*
 * Line Following PID Variables
 */
float error = 0, prev_error = 0, difference, cumulative_error, correction;

/*
 * Union containing line sensor readings
 */
line_sensor_array line_sensor_readings;

/*
 * Maze solving states
 */
typedef enum
{
    FOLLOW_LINE,
    TURN_LEFT,
    TURN_RIGHT,
    DETECT_INTERSECTION,
    END_OF_MAZE,
    REACHED_GOAL
} MazeState;

MazeState current_state = FOLLOW_LINE;

#define MAX_MAZE_PATH_LENGTH 100 // Adjust the maximum length of the maze path as needed

typedef enum
{
    LEFT_TURN,
    RIGHT_TURN
} TurnDirection;

typedef struct
{
    TurnDirection direction;
} MazeTurn;

MazeTurn maze_path[MAX_MAZE_PATH_LENGTH];
int maze_path_length = 0;

// Function to record a left turn in the maze path
void record_left_turn()
{
    if (maze_path_length < MAX_MAZE_PATH_LENGTH)
    {
        maze_path[maze_path_length].direction = LEFT_TURN;
        maze_path_length++;
    }
    else
    {
        // Handle the case when the maze path array is full
        // You may want to implement error handling or dynamic memory allocation here
    }
}

// Function to record a right turn in the maze path
void record_right_turn()
{
    if (maze_path_length < MAX_MAZE_PATH_LENGTH)
    {
        maze_path[maze_path_length].direction = RIGHT_TURN;
        maze_path_length++;
    }
    else
    {
        // Handle the case when the maze path array is full
        // You may want to implement error handling or dynamic memory allocation here
    }
}

// Intersection detection based on LSA weights
bool detect_intersection(line_sensor_array readings)
{
    // Check for the pattern of sensor readings indicating an intersection
    // For example, if the weights of the middle sensors are higher than the weights of the outer sensors, it may indicate an intersection
    if (readings.adc_reading[2] > readings.adc_reading[1] && readings.adc_reading[2] > readings.adc_reading[3])
    {
        return true;
    }
    return false;
}

// Function to calculate error based on black boundary
void calculate_error()
{
    int all_black_flag = 1; // assuming initially all black condition
    float weighted_sum = 0, sum = 0;
    float pos = 0;
    int k = 0;

    for (int i = 0; i < 5; i++)
    {
        if (line_sensor_readings.adc_reading[i] > BLACK_BOUNDARY) // Check for black instead of white
        {
            all_black_flag = 0;
        }
        if (line_sensor_readings.adc_reading[i] > BLACK_BOUNDARY) // Check for black instead of white
        {
            k = 1;
        }
        if (line_sensor_readings.adc_reading[i] < BLACK_BOUNDARY) // Check for black instead of white
        {
            k = 0;
        }
        weighted_sum += (float)(weights[i]) * k;
        sum = sum + k;
    }

    if (sum != 0) // sum can never be 0 but just for safety purposes
    {
        pos = (weighted_sum - 1) / sum; // This will give us the position with respect to the line. if +ve then bot is facing left and if -ve the bot is facing to right.
    }

    if (all_black_flag == 1) // If all black then we check for previous error to assign current error.
    {
        if (prev_error > 0)
        {
            error = 2.5;
        }
        else
        {
            error = -2.5;
        }
    }
    else
    {
        error = pos;
    }
}

void follow_line()
{
    set_motor_speed(motor_a_0, MOTOR_FORWARD, left_duty_cycle);
    set_motor_speed(motor_a_1, MOTOR_FORWARD, right_duty_cycle);
}

void turn_left()
{
    set_motor_speed(motor_a_0, MOTOR_BACKWARD, left_duty_cycle);
    set_motor_speed(motor_a_1, MOTOR_FORWARD, right_duty_cycle);
}

void turn_right()
{
    set_motor_speed(motor_a_0, MOTOR_FORWARD, left_duty_cycle);
    set_motor_speed(motor_a_1, MOTOR_BACKWARD, right_duty_cycle);
}

void stop_robot()
{
    set_motor_speed(motor_a_0, MOTOR_STOP, 0);
    set_motor_speed(motor_a_1, MOTOR_STOP, 0);
}

void maze_solve_task(void *arg)
{
    motor_handle_t motor_a_0, motor_a_1;
    ESP_ERROR_CHECK(enable_motor_driver(&motor_a_0, MOTOR_A_0));
    ESP_ERROR_CHECK(enable_motor_driver(&motor_a_1, MOTOR_A_1));
    adc_handle_t line_sensor;
    ESP_ERROR_CHECK(enable_line_sensor(&line_sensor));
    ESP_ERROR_CHECK(enable_bar_graph());
#ifdef CONFIG_ENABLE_OLED
    // Initialising the OLED
    ESP_ERROR_CHECK(init_oled());
    vTaskDelay(100);

    // Clearing the screen
    lv_obj_clean(lv_scr_act());

#endif

    bool exploration_done = false; // Flag to indicate exploration completion
    int path_index = 0;            // Index to iterate through the recorded path

    while (true)
    {
        line_sensor_readings = read_line_sensor(line_sensor);
        for (int i = 0; i < 5; i++)
        {
            line_sensor_readings.adc_reading[i] = bound(line_sensor_readings.adc_reading[i], BLACK_MARGIN, WHITE_MARGIN);
            line_sensor_readings.adc_reading[i] = map(line_sensor_readings.adc_reading[i], BLACK_MARGIN, WHITE_MARGIN, bound_LSA_LOW, bound_LSA_HIGH);
        }

        calculate_error();

        switch (current_state)
        {
        case FOLLOW_LINE:
            if (error > 0)
            {
                follow_line();
            }
            else if (error < 0)
            {
                turn_left();
                record_left_turn(); // Record left turn
#ifdef CONFIG_ENABLE_OLED
                // Display message on OLED
                lv_label_set_text(label, "Turning left...");
                lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);
#endif
            }
            else
            {
                turn_right();
                record_right_turn(); // Record right turn
#ifdef CONFIG_ENABLE_OLED
                // Display message on OLED
                lv_label_set_text(label, "Turning right...");
                lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);
#endif
            }

            // Check for intersection
            if (detect_intersection(line_sensor_readings))
            {
                if (exploration_done)
                {
                    // Use recorded path for solving
                    current_state = maze_path[path_index].direction == LEFT_TURN ? TURN_LEFT : TURN_RIGHT;
                    path_index++; // Move to the next turn in the recorded path
                }
                else
                {
                    // Continue exploration (existing logic for recording turns)
                }
            }
            break;

        case TURN_LEFT:
            turn_left();
            vTaskDelay(1000 / portTICK_PERIOD_MS); // Adjust delay for turning left
            current_state = FOLLOW_LINE;
            break;

        case TURN_RIGHT:
            turn_right();
            vTaskDelay(1000 / portTICK_PERIOD_MS); // Adjust delay for turning right
            current_state = FOLLOW_LINE;
            break;

        case DETECT_INTERSECTION:
            // Check for goal (black square)
            if (detect_goal(line_sensor_readings))
            {
                current_state = REACHED_GOAL;
#ifdef CONFIG_ENABLE_OLED
                // Display message on OLED
                lv_label_set_text(label, "Goal reached!");
                lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);
#endif
                break;
            }
            // Check for end of maze
            if (detect_end_of_maze(line_sensor_readings))
            {
                current_state = END_OF_MAZE;
#ifdef CONFIG_ENABLE_OLED
                // Display message on OLED
                lv_label_set_text(label, "End of maze detected!");
                lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);
#endif
                break;
            }
            // Otherwise, continue following the line
            follow_line();
            break;

        case END_OF_MAZE:
            // Stop the robot
            stop_robot();
            // Perform action to turn back or navigate back to the path
            // For simplicity, let's assume turning back
            turn_left();                           // Turn around 180 degrees
            vTaskDelay(2000 / portTICK_PERIOD_MS); // Adjust delay for turning around
            current_state = FOLLOW_LINE;           // Return to following the line
            break;

        case REACHED_GOAL:
            // Stop the robot at the goal
            stop_robot();
            // You may perform additional actions here, such as signaling completion
            // Then, exit the maze solving task
            vTaskDelete(NULL);
            break;

        default:
            break;
        }

        // Check for exploration completion and introduce delay
        if (!exploration_done && detect_end_of_maze(line_sensor_readings))
        {
            exploration_done = true;
#ifdef CONFIG_ENABLE_OLED
            // Display message on OLED
            lv_label_set_text(label, "Exploration complete. Solving maze...");
            lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);
#endif
            vTaskDelay(20000 / portTICK_PERIOD_MS); // Delay for 20 seconds
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}

void app_main()
{
    xTaskCreate(&maze_solve_task, "maze_solve_task", 4096, NULL, 1, NULL);
    start_tuning_http_server();
}
