# Conexio Stratus nRF9160 watchdog Timer sample

This repository contains example firmware for enabling and kicking software watchdog timer.

Sample Output
=============
```
*** Booting Zephyr OS build v2.6.99-ncs1  ***
<inf> wdtimer_app: Stratus watchdog timer sample
<inf> watchdog: Watchdog timeout installed. Timeout: 60000 ms
<inf> watchdog: Watchdog timer started
<dbg> watchdog.watchdog_feed_enable: Watchdog feed enabled. Timeout: 30000 ms
<dbg> watchdog.secondary_feed_worker: Feeding watchdog
<dbg> watchdog.secondary_feed_worker: Feeding watchdog
```