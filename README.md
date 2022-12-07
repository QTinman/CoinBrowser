# CoinBrowser
CoinBrowser is a tool for Freqtrade where the program writes pairs into text file to be used with spesific exchange, these pairs file is also used in the simulator.

Data for this program is from CoinMarkedCap API.

If database does not exist upon startup the program creates coinhistory.db database from exiting json file.

The database is updated into own table from existing json file right before new json file is downloaded.

Database can then be selected from droppdown in mainwindow.


![Welcome screen](https://github.com/QTinman/CoinBrowser/blob/main/screencap.png)


The txt files starting with "raw_" is created with command "docker-compose run --rm freqtrade list-pairs --exchange binance" and the table from the output copied to new file, example raw_binance.txt.

Invest simulator lets you do fictional crypto buys.

To get crypto in simulator dropdown list do write to file in mainwindow and point to that file in settings.

![Welcome screen](https://github.com/QTinman/CoinBrowser/blob/main/simulator.png)

Crypto invest simulator

![Welcome screen](https://github.com/QTinman/CoinBrowser/blob/main/settings.png)

Typical settings.
