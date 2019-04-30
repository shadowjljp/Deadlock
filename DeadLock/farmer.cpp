#include <pthread.h>
#include <cstdlib>
#include <iostream>
#include <time.h>
#include <unistd.h>
enum Direction{NORTH, SOUTH, NONE};
int farmerAmountOnBridge = 0;
Direction currentDirection = NONE;
int const FarmerNum = 10;
pthread_mutex_t farmerIndexLock[FarmerNum], locationChange, outputLock;
int farmerIndexLockCircularPointer = 0;
pthread_cond_t farmer;

/*
  假設南北各有10位農民要過橋並且每個人過橋需要花費1~3秒  
 
  每位農民會在0~10秒內的隨機時間發出想過橋的要求         
 */
 
void *southFarmer(void *param){
    srand(time(NULL) ^ (int)pthread_self());
    // sleep 0~10 seconds
    sleep(((double)rand() / RAND_MAX) * 10);
    pthread_mutex_lock(&outputLock);
    std::cout << "        South farmer " << *((int*)param) << "\twaiting to go on the bridge" << std::endl;
    pthread_mutex_unlock(&outputLock);
    // prevent race condition of "currentDirction", "farmerAmountOnBridge" and "farmerIndexLockPointer"
    pthread_mutex_lock(&locationChange);

    // if the direction is not correct, then wait and release "locationChange" temporary
    while (currentDirection == NORTH){
        pthread_cond_wait(&farmer, &locationChange);}

    // farmer get on the bridge, and walking time set 1~3 seconds
    pthread_mutex_lock(&outputLock);
    double walkingTime = ((double)rand() / RAND_MAX) * 2 + 1;
    std::cout << "        South farmer " << *((int*)param) << "\tgo on the bridge and will walking for " << walkingTime << " seconds" << std::endl;
    pthread_mutex_unlock(&outputLock);
    farmerAmountOnBridge++;

    // if he is the first farmer of this direction, then change direction
    if(farmerAmountOnBridge == 1)
        currentDirection = SOUTH;

    // get this farmer's index and lock it, so next farmer know that this farmer is on the birdge
    int farmerIndex = farmerIndexLockCircularPointer % FarmerNum;
    pthread_mutex_lock(farmerIndexLock + farmerIndex);
    farmerIndexLockCircularPointer++;

    pthread_mutex_unlock(&locationChange);

    // need 1~3 seconds to across bridge
    sleep(walkingTime);

    // wait previous farmer arrive side
    int previousFarmerIndex = (farmerIndex + 9) % FarmerNum;
    pthread_mutex_lock(farmerIndexLock + previousFarmerIndex);
    pthread_mutex_unlock(farmerIndexLock + previousFarmerIndex);

    // prevent race condition of "currentDirction" and "farmerAmountOnBridge"
    pthread_mutex_lock(&locationChange);

    farmerAmountOnBridge--;

    // if he is the last farmer of this direction, then change direction to none
    if(farmerAmountOnBridge == 0){
        currentDirection = NONE;
        // notify other farmer which is at waiting state to start
        pthread_cond_broadcast(&farmer);
    }
    pthread_mutex_unlock(&locationChange);

    pthread_mutex_lock(&outputLock);
    std::cout << "        South farmer " << *((int*)param) << "\tarrived" << std::endl;
    pthread_mutex_unlock(&outputLock);

    // notify next farmer that he arrived
    pthread_mutex_unlock(farmerIndexLock + farmerIndex);
    int* i = (int*)param;
    free(i);
    pthread_exit(0);
}

void *northFarmer(void *param){
    srand(time(NULL) ^ (int)pthread_self());
    // sleep 0~10 seconds
    sleep(((double)rand() / RAND_MAX) * 10);
    pthread_mutex_lock(&outputLock);
    std::cout << "North         farmer " << *((int*)param) << "\twaiting to go on the bridge" << std::endl;
    pthread_mutex_unlock(&outputLock);

    // prevent race condition of "currentDirction" and "farmerAmountOnBridge"
    pthread_mutex_lock(&locationChange);

    // if the direction is not correct, then wait and release "locationChange" temporary
    while (currentDirection == SOUTH){
        pthread_cond_wait(&farmer, &locationChange);}

    // farmer get on the bridge, and walking time set 1~3 seconds
    pthread_mutex_lock(&outputLock);
    double walkingTime = ((double)rand() / RAND_MAX) * 3 + 1;
    std::cout << "North         farmer " << *((int*)param) << "\tgo on the bridge and will walking for " << walkingTime << " seconds" << std::endl;
    pthread_mutex_unlock(&outputLock);
    farmerAmountOnBridge++;

    // if he is the first farmer of this direction, then change direction
    if(farmerAmountOnBridge == 1)
        currentDirection = NORTH;

    // get this farmer's index and lock it, so next farmer know that this farmer is on the birdge
    int farmerIndex = farmerIndexLockCircularPointer % FarmerNum;
    pthread_mutex_lock(farmerIndexLock + farmerIndex);
    farmerIndexLockCircularPointer++;

    pthread_mutex_unlock(&locationChange);


    // need 1~3 seconds to across bridge
    sleep(((double)rand() / RAND_MAX) * 3 + 1);

    // wait previous farmer arrive side
    int previousFarmerIndex = (farmerIndex + 9) % FarmerNum;
    pthread_mutex_lock(farmerIndexLock + previousFarmerIndex);
    pthread_mutex_unlock(farmerIndexLock + previousFarmerIndex);

    // prevent race condition of "currentDirction" and "farmerAmountOnBridge"
    pthread_mutex_lock(&locationChange);

    farmerAmountOnBridge--;

    // if he is the last farmer of this direction, then change direction to none
    if(farmerAmountOnBridge == 0){
        currentDirection = NONE;
        // notify other farmer which is at waiting state to start
        pthread_cond_broadcast(&farmer);
    }
    pthread_mutex_unlock(&locationChange);

    pthread_mutex_lock(&outputLock);
    std::cout << "North         farmer " << *((int*)param) << "\tarrived." << std::endl;
    pthread_mutex_unlock(&outputLock);

    // notify next farmer that he arrived
    pthread_mutex_unlock(farmerIndexLock + farmerIndex);
    int* i = (int*)param;
    free(i);
    pthread_exit(0);
}

int main(int argc, char **argv) {
    srand(time(NULL));
    for (int i = 0; i < 11; i++ ) {
        pthread_mutex_init(farmerIndexLock + i, NULL);
    }
    pthread_mutex_init(&locationChange, NULL);
    pthread_cond_init(&farmer, NULL);

    // Create 10 south farmers
    pthread_t southTids[FarmerNum];
    for (int i = 0; i < FarmerNum; ++i ) {
        int * j = new int(i + 1);
        pthread_create(southTids + i, NULL, southFarmer, (void*)j);
    }

    // Create 10 north farmers
    pthread_t northTids[FarmerNum];
    for (int i = 0; i < FarmerNum; ++i) {
        int * j = new int(i + 1);
        pthread_create(northTids + i, NULL, northFarmer, (void*)j);
    }

    // Wait for all farmers
    for (int i = 0; i < FarmerNum; ++i ) {
        pthread_join(southTids[i], NULL);
    }
    for (int i = 0; i < FarmerNum; ++i) {
        pthread_join(northTids[i], NULL);
    }

    return 0;
}
