Beefore converting a file, you have to create (if not already present) or adjust all the first descriptive row names.
Remind that spaces are not allowed, and the names must match with the names in the new ini files

Probably you need to do something like this: 

php csv2ini.php /path/to/file.csv | grep -v nused > /path/to/file.ini

to remove all the unused fields.

NOTE:

There are some csv files (like terraintable.ini etc.) that have multiple values as keys, this is not possible in the ini format.
csv2ini try to identify this kind of files (by matching the first value of the second and third row) and convert the values like this:

csv file:

key1,value1,value2,value3
xx,0,1,2
xx,1,1,2

to ini:

[xx]
value1 = 0,1
value2 = 1,1
value3 = 2,2
 
