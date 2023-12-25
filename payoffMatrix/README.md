# payoffMatrix format defination

```txt
Donor Recipient:b c beta gamma lambda p0,NR,SR,AR,UR
C,-c:b,beta-c:b-gamma,-c b,beta-c:b-gamma
DISC,0:0,p0*(beta-c):p0*(b-gamma),(beta-c)/2:(b-gamma)/2,beta-c:b-gamma
NDISC,-c:b,(beta-c)/2:(b-gamma)/2,(1-p0)*(-c)+p0*beta:(1-p0)*b+p0*(-gamma),beta:-gamma
D,0:0,0:0,beta:-gamma,beta:-gamma
```

The file format is based on the comma-separated values (CSV) format (using `,` as the delimiter string)
, and has column names and row names such as 'C' or 'NR'. It also stores a list of player names and the list of variable names used in the file, such as 'Donor Recipient:b c beta gamma lambda p0' (located 0,0)

elements:
- player name list
- variable name list
- row name list
- column name list
- payoff matrix for each player

## player name list and variable name list

They are separated by the symbol `:`.

And the elements in the list are separated by the symbol ` `.

## row names

The strategies of the player in the first place of player name list

## column names

The strategies of the player in the second place of player name list

## payoff matrix

each cell in payoff matrix is a binary tuple, the first element is the payoff of the first player, the second element is the payoff of the second player.

Different players' payoff expressions are separated by the symbol `:`.

The expressions is allowed to contain blank spaces.

## how to create

The payoff matrix config file can be created by the ipynb file in `./formula`.

The content of the file is determined by the symbolic computing procedure in the ipynb file.