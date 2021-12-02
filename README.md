# CoinBrowser
CoinBrowser is a tool for Freqtrade where the program writes pairs into text file to be used with spesific exchange.

Data for this program is from CoinMarkedCap API. API key is needed for downloading the json file and placed in command files.

If database does not exist upon startup the program creates coinhistory.db database from exiting json file.

The database is updated into own table from existing json file right before new json file is downloaded.

Database can then be selected from droppdown in mainwindow.


![Welcome screen](https://github.com/QTinman/CoinBrowser/blob/main/screencap.png)


The txt files starting with "raw_" is created with command "docker-compose run --rm freqtrade list-pairs --exchange binance" and the table from the output copied to new file, example raw_binance.txt.

![Welcome screen](https://github.com/QTinman/CoinBrowser/blob/main/simulator.png)

Crypto invest simulator

![Welcome screen](https://github.com/QTinman/CoinBrowser/blob/main/settings.png)

Typical settings.
