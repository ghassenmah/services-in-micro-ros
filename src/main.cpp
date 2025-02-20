#include <Arduino.h>
#include <micro_ros_platformio.h>
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/bool.h>
#include <std_srvs/srv/set_bool.h>
#include <rosidl_runtime_c/string.h>  // Pour manipuler rosidl_runtime_c__String

// Définir la pin de la LED
#define LED_PIN A0  // Pin PA0 de la STM32

// Variables Micro-ROS
rcl_node_t node;  // Nœud Micro-ROS
rcl_service_t led_service;  // Service pour contrôler la LED
rclc_executor_t executor;  // Executor pour gérer les tâches
rclc_support_t support;  // Support de Micro-ROS
rcl_allocator_t allocator;  // Allocateur
rcl_timer_t timer;  // Timer (si nécessaire pour publication périodique)
std_srvs__srv__SetBool_Request request_msg;  // Message de requête pour activer/désactiver la LED
std_srvs__srv__SetBool_Response response_msg;  // Message de réponse pour confirmer l'action

// Fonction d'erreur en cas de problème
void error_loop() {
  while (1) {
    delay(100);  // Boucle infinie pour signaler l'erreur
  }
}

// Fonction pour assigner des chaînes à `rosidl_runtime_c__String`
void assign_string(rosidl_runtime_c__String * str, const char * data) {
  str->data = (char *)data;  // Assignation directe
  str->size = strlen(data);  // Définir la taille de la chaîne
  str->capacity = str->size + 1;  // Réserver de l'espace pour la fin de chaîne '\0'
}

// Callback du service pour activer/désactiver la LED
void handle_led_service(const void * request, void * response) {
  const std_srvs__srv__SetBool_Request * req = (const std_srvs__srv__SetBool_Request *)request;
  std_srvs__srv__SetBool_Response * res = (std_srvs__srv__SetBool_Response *)response;

  // Activer ou désactiver la LED en fonction de la requête
  if (req->data) {
    digitalWrite(LED_PIN, HIGH);  // Allumer la LED
    res->success = true;
    assign_string(&res->message, "LED turned ON");
  } else {
    digitalWrite(LED_PIN, LOW);  // Éteindre la LED
    res->success = true;
    assign_string(&res->message, "LED turned OFF");
  }

  // Afficher l'état de la LED sur le moniteur série
  Serial.print("LED State: ");
  Serial.println(req->data ? "ON" : "OFF");
}

void setup() {
  // Initialisation de la communication série
  Serial.begin(115200);
  set_microros_serial_transports(Serial);  // Définir les transports série pour Micro-ROS
  delay(2000);  // Attendre la communication avec l'agent ROS2

  // Initialiser la pin de la LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);  // Éteindre la LED au démarrage

  // Initialiser l'allocateur pour Micro-ROS
  allocator = rcl_get_default_allocator();

  // Initialiser le support Micro-ROS
  rcl_ret_t rc = rclc_support_init(&support, 0, NULL, &allocator);
  if (rc != RCL_RET_OK) {
    error_loop();
  }

  // Initialiser le nœud Micro-ROS
  rc = rclc_node_init_default(&node, "stm32_led_node", "", &support);
  if (rc != RCL_RET_OK) {
    error_loop();
  }

  // Initialiser le service pour contrôler la LED
  rc = rclc_service_init_default(
    &led_service,
    &node,
    ROSIDL_GET_SRV_TYPE_SUPPORT(std_srvs, srv, SetBool),
    "control_led"
  );
  if (rc != RCL_RET_OK) {
    error_loop();
  }

  // Initialiser l'exécuteur pour gérer le service
  rc = rclc_executor_init(&executor, &support.context, 1, &allocator);  // 1 tâche : le service
  if (rc != RCL_RET_OK) {
    error_loop();
  }

  // Ajouter le service à l'exécuteur
  rc = rclc_executor_add_service(&executor, &led_service, &request_msg, &response_msg, handle_led_service);
  if (rc != RCL_RET_OK) {
    error_loop();
  }
}

void loop() {
  // Exécuter l'exécuteur Micro-ROS pour gérer les services
  rcl_ret_t rc = rclc_executor_spin_some(&executor, RCUTILS_MS_TO_NS(100));  // Exécuter les tâches disponibles
  if (rc != RCL_RET_OK) {
    error_loop();
  }

  delay(10);  // Petite pause pour permettre à l'exécuteur de tourner
}
