======================================Tema2 SO======================================

Am folosit urmatoarele 2 structuri:
    -Structura "thread" :pe langa campurile obisnuite(id,priority,handler si io)am adaugat un semafor
        pentru a putea planifica ordinea de executie a threadului  si trei pointeri catre aceasta structura cu urmatoarele scopuri:
            -campul "urm_priority pointeaza catre threadurile ce se afla in coada "priority_queue"(coada principala), in functie de prioritatea lor
            -campul "urm_threads" pointeaza catre toate threadurile create in ordinea fork-urilor,campul fiind folosit    
                    de catre lista "threads_queue" in functia so_end pentru a astepta terminarea tuturor threadurilor
            -campul "urm_free" pointeaza catre toate threadurile create in ordinea fork-urilor,campul fiind folosit    
                    de catre lista "free_queue" in functia so_end pentru a elibera memoria ocupata de threaduri
    -Structura "scheduler" contine numarul de evenimente suportate,cuanta de timp si un pointer la strucura thread prin care retin ce thread ruleaza

Pentru a contoriza numarul de threaduri in coada cu prioritati "priority_queue" am folosit variabila "count"
Am folosit un vector de threaduri in care am pus threadurile blocate de functia so_wait

Pe langa functiile principale am mai implementat  
        -functia "add_queue" care adauga threaduri in coada "priority_queue" in functie de prioritatea lor
        -functia "extract" extrage primul thread din coada "priority_queue"
        -functia "switchthread" -extrage primul thread din coada "priority_queue"
                                -il pune in running,notificandu l ca poate rula cu functia "sem_post"
        -functia "scheduler" planifica ce thread va rula facand urmatoarele verificari:
                                -in caz ca sunt threaduri in coada cu prioritati:
                                    daca threadul curent nu a terminat dar exista alt thread cu prioritate mai mare, fac schimbul:
                                            adaug threadul curent in coada,il blockez cu functia sem_wait si pornesc primul thread din coada
                                    daca thredul curent a terminat, dar are cea mai mare prioritate, continua tot el, adaugandu-i timp
                                    altfel adaug threadul curent in coada,il blockez cu functia sem_wait si pornesc primul thread din coada
                                    

                                    

 so_init: Verific parametri primiti,iar daca totul este ok aloc memorie pentru scheduler si initilizez variabilele globale

 so_fork: aloc memorie unui nou thread si il initializez cu parametri primiti
          il adaung in listele unde nu conteaza prioritatile doar prezenta threadurilor (listele vor fi utizilate la functia so_end)
                    (threads_queue si free_queue)
          initializez semaforul pentru fiecare thread (cu ajutorul lui voi determina planificarea threadurilor)
          adaug in coada cu priotitati si creez thread ul(functia primita ca argument este "start_thread")
          functia start_thread: blocheaza threadul pana este planificat, cu functia sem_wait
                              : dupa ce este notificat de alt thread si poate rula executa rutina primita
                              : dupa ce isi termina executia si mai exista thread uri in coada cu prioritati se apeleaza functia switchthread
          in caz ca este primul element din coada cu prioritati se pune direct in running cu functia switchthread
          altfel,scad cuanta de timp si apelez planificatorul(functia scheduler).

so_end: parcurg lista "threads_queue" pentru a astepta terminarea tuturor threadurilor
        parcurg lista "free_queue" pentru a elibera memoria ocupata de fiecare thread cu semaforul respectiv
        eliberez scheduler ul

so_wait: setez threadului care ruleaza io ul respectiv
         il adaug in vectorul "blocked_queue" 
         il blochez cu functia sem_wait si apelez functia "switchthread" pentru a rula un nou thread
         in caz ca vectorul "blocked_queue" ramane fara memorie, il realoc


so_signal:
        parcurg vectorul "blocked_queue",iar daca threadul are acelasi io cu io ul primit ca argument  il adaug in coada cu prioritati
        fac replanificarea apeland functia "scheduler"




