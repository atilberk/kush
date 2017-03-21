COMP 304, Project 1: KU shell
# KUSH Report
Atılberk Çelebi,
Umur Kağan Şen

The project mostly consists of children's interaction with each other and their parent. There are four main children we use throughout the program: First one, _execchild_, is to execute and print the output to the parent. Second child, _whichchild_, is to use `which` command to find the location of the corresponding command. The third one, _whereischild_, is to check if the entered command is valid or not. Finally the fourth child, _pipechild_, is to use pipe command (eg. `ls | less` ) in order to do the first part and give the output of it as the input of the second command. There are five pipes to hold this structure `pfd`, `pfd2`, `pfd3`, and `pfd4`. This structure helps us to complete the first part of the project. We also added `clear` and `cd` commands to provide more controlled and clean environment inside the shell. We imitated the bash’s application on those, for instance `cd` without any argument changes the current directory to user’s home directory.

In the second part, we first check if there is any pipe, ` | `. If there is, then we initially create _pipechild_ in order to complete the first command. Then it passes the output to `pfd4` so that _execchild_ can use this information as its standard input. When _execchild_ finishes the execution, it passes its output to the parent so that parent can print the output. During these processes, we check if there are any ` > ` or ` >> ` marks by using `checkForArrow()` and `checkForDoubleArrow()` methods. If we find any, then, as usual, first we execute the command with _execchild_, pass the output to parent. Parent uses `writeInto()` command to write the execution output to a file.

In the third part, we create a kush crontab document in the kush directory (` $HOME/.kush `) in order to keep the crontab commands. Whenever a trash command is given to kush, we write into this document to keep track of the jobs so that we can delete one particular crontab job given its path.

Our own command implemented in kush is `dizipub`. This command takes the name of a tv show and season and episode information, then opens that particular episode of the show on Firefox. There are three options for executing `dizipub`:
* `dizipub <series> [<season> <episode>]` : With the given name of tv series, the season and the episode, it opens at Firefox a dizipub.com tab with corresponding TV series and episode.
* `dizipub <series> [<next>]` : With the given name of tv series, it opens the next episode you will watch. We keep the information of the episodes you watched by using kush so that you don’t have to type the episode and season over and over again but just type next.
* `dizipub -l` : Lists previously watched tv series along with their season and episode information in chronological order.
* Mistyping on `dizipub` command: It gives the possible type of commands that user can type in, such as the ones above.

We again keep the list of episodes the user watched in a document in kush directory.

In the last part, we have written _schedInfo_ module as a separate C file ( `schedInfo.c` ). However, kush knows that file. In its first load, kush copies the file and its `Makefile` into kush directory and compiles it using `make` command. Then `schedInfo` command in kush takes proper arguments and loads the module if it has not yet been loaded. In case there is a different process id given, then kush unloads the module and reloads for the new process id. 
