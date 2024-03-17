testNumber=1
filePaht=$1

evaulate() {
    if [ "$testVal" = "$val" ]; then
        echo "Test "$testNumber" pass"
    else
        echo "Test "$testNumber" did not pass"
        echo ""
        echo "Test val:"
        echo "$testVal"
        echo "val:"
        echo "$val"

    fi
}

testVal=$("$filePaht" Trader1 cryptoexchange.log)
val="Trader1;2024-01-15 15:30:42;EUR;-2000.0000
Trader1;2024-01-16 18:06:32;USD;-3000.0000
Trader1;2024-01-20 11:43:02;ETH;1.9417
Trader1;2024-01-22 09:17:40;ETH;10.9537"

testNumber=1
evaulate

testVal=$("$filePaht" list Trader1 cryptoexchange.log)
val="Trader1;2024-01-15 15:30:42;EUR;-2000.0000
Trader1;2024-01-16 18:06:32;USD;-3000.0000
Trader1;2024-01-20 11:43:02;ETH;1.9417
Trader1;2024-01-22 09:17:40;ETH;10.9537"

testNumber=2
evaulate

testVal=$("$filePaht" -c GBP Trader1 cryptoexchange.log)
val=""

testNumber=3
evaulate

testVal=$("$filePaht" list-currency Trader1 cryptoexchange.log)
val="ETH
EUR
USD"

testNumber=4
evaulate

testVal=$("$filePaht" status Trader1 cryptoexchange.log)
val="ETH : 12.8954
EUR : -2000.0000
USD : -3000.0000"

testNumber=5
evaulate

testVal=$("$filePaht" -b "2024-01-22 09:17:40" status Trader1 cryptoexchange.log)
val="ETH : 1.9417
EUR : -2000.0000
USD : -3000.0000"

testNumber=6
evaulate

testVal=$("$filePaht" -a "2024-01-15 16:00:00" -b "2024-01-22 09:17:41" status Trader1 cryptoexchange.log)
val="ETH : 12.8954
USD : -3000.0000"

testNumber=7
evaulate

export XTF_PROFIT=20
testVal=$("$filePaht" profit Trader1 cryptoexchange.log)
val="ETH : 15.4744
EUR : -2000.0000
USD : -3000.0000"

touch result7.res
touch reference7.txt
echo "$testVal" >result7.res
echo "$val" >reference7.txt

testNumber=8
evaulate

export XTF_PROFIT=40
testVal=$("$filePaht" profit Trader1 cryptoexchange.log)
val="ETH : 18.0535
EUR : -2000.0000
USD : -3000.0000"
testNumber=9
evaulate
