# CoinBrowser
CoinBrowser is a tool for Freqtrade where the program writes pairs into text file to be used with spesific exchange.

Data for this program is from CoinMarkedCap API. API key is needed for downloading the json file and placed in command files.

If database does not exist upon startup the program creates coinhistory.db database and uses the data from exiting json file.

The database can be updated by checking UpdateDB and will it then upgrade from existing json file.

Typically it is good to have the database few hours older than the json file as the DB should be few hours older for some calculations.
The settings are little quirky and the program might need to be restarted after setting stake coin.

cUrl is needed for using json download scripts.

![Welcome screen](https://github.com/QTinman/CoinBrowser/blob/main/screencap.png)


About files
The txt files with _raw_ in the name is created with command "freqtrade list-pairs -c config_ETH_Bittrex.json | grep ETH" and copied to file with this format.
the get_ files are command scripts to dl jason files. 

![Welcome screen](https://github.com/QTinman/CoinBrowser/blob/main/settings.png)
Typical settings.
