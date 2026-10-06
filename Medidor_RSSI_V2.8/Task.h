
Scheduler PDR;  // Tareas relacionadas a la prueba de red PDR
// --------------------------------------------------------------------------
// Prototipos de funciones
// --------------------------------------------------------------------------

void config_task();
void decrementarEspera();
void intentarEnvioPDR();
void EsperarAck();
void tareaFinalizar();
void PaqueteSalida();

// Tareas de PDR
Task tDecrementarEspera(1000, TASK_FOREVER, &decrementarEspera, &PDR, true, NULL, &intentarEnvioPDR);

Task tEsperarAck(500, TASK_FOREVER, &EsperarAck, &PDR, false);

Task tEnvio(10000, TASK_FOREVER, &PaqueteSalida, &PDR, false);          //envia el paquete de salida cada 10 segundos

/*Task(unsigned long aInterval,           1. aInterval is in milliseconds (or microseconds) (default = 0)
              long aIterations,           2. aIteration in number of times, -1 for indefinite execution (default = -1)
           void (*aCallback)(),           3. aCallback is a pointer to a void callback method without parameters (default = NULL)
         Scheduler* aScheduler,           4. aScheduler – optional reference to existing scheduler. If supplied (not NULL) this task will be appended to the task chain of the current scheduler). (default = NULL)
                  bool aEnable,           5. aEnable – optional. Value of true will create task enabled. (default = false)
           bool (*aOnEnable)(),           6. aOnEnable is a pointer to a bool callback method without parameters, invoked when task is enabled. If OnEnable method returns true , task is enabled. If OnEnable method return false , task remains disabled (default = NULL)
         void (*aOnDisable)());           7. aOnDisable is a pointer to a void callback method without parameters, invoked when task is disabled (default = NULL)
           
  All tasks are created disabled by default (unless aEnable = true). You have to explicitly enable the task for execution.
*/