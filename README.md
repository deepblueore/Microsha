# Microsha
Компиляция:  
g++ microsha2.cpp -o microsha_valgrind (valgrind ./microsha_valgrind для запуска с valgrind)  
g++ -fsanitize=address microsha2.cpp -o microsha_fsanitize (./microsha_fsanitize для запуска с -fsanitize=address)  

Почему-то работает ls -l /* /* .h  
