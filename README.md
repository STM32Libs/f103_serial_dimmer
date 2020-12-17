# blue pill serial dimmer
Blue pill STM32F103 example with Arduino framework in platformio

# Flashing
* with stlink, move boot0 jumper to 1 : programming mode

    <img src ="./media/boot.png">

* press reset
* set the jumper back

    <img src ="./media/operation.png">
# serial command examples

    {"all":100}
    {"all":1000}
    {"all":9000}
    {"all":10000}
    {"list":[1000,2000,3000,4000,5000]}
    {"list":[1000,2000,3000,4000,5000,6000,7000,8000]}
    {"L2":3000,"L5":4000}
    {"get":"status"}
