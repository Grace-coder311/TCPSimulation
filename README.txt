Lab8 Network Programming

1) Use command $make to create drone executable
2) Run executable with $drone8 <portNumber>
3) Messages can be sent and received between drones using the command line and network
4) The receiving drone will print the message iff it has the correct port value, TTL >= 0, and the distance is close enough

_____________________
Testing Assumptions:

On 3/30/23, we will have 5 rows and 3 columns for Lab7 demo day

_____________________
Assumptions for Lab7
Here are some answers I shared with your classmates

1) remember that a message with move:x in it will NOT have a msg: field in it
2) remember that a message with move:x is NEVER forwarded
3) remember that a message with move:x is received by destination regardless of the distance (square 1 can receive a move:x from square 1000, for instance)

_____________________
Notes:
*config file is in the same directory as the executable. We will NOT prompt the user for the filename
_____________________
References:
Using static variables: https://www.it.uc3m.es/pbasanta/asng/course_notes/variables_en.html
Debug using gdb guide: https://u.osu.edu/cstutorials/2018/09/28/how-to-debug-c-program-using-gdb-in-6-simple-steps/
Used for reading files in C line-by-line: https://solarianprogrammer.com/2019/04/03/c-programming-read-file-lines-fgets-getline-implement-portable-getline/#:~:text=The%20standard%20way%20of%20reading,GitHub%20repo%20for%20this%20article.
Linked List: https://www.tutorialspoint.com/data_structures_algorithms/linked_list_program_in_c.htm'
Function 'replace_str' https://www.linuxquestions.org/questions/programming-9/replace-a-substring-with-another-string-in-c-170076/
Dynamic Mem for Struct: https://stackoverflow.com/questions/13590812/c-freeing-structs
