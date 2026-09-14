from machine import ADC, Pin
import time
adc = ADC(Pin(47))
while True:
    adc_value = adc.read_u16()
    voltage = (adc_value / 65535) * 3.3
    print("voltage:", voltage, "V")
    time.sleep(0.5)
