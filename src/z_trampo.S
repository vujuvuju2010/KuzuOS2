section .text
global z_trampo
extern enter_user_mode
z_trampo:
        ; ya twinnitto twinitti normalde burda jmp enter_user_mode vardı 
        ; ama int 13 hatası tükürünce bunu değiştirdem gerekirse geri al
        ; AMA ALIRSA ENTER_USER_MODE UN BASINDAKILERI 4 VE 8 E GERI CEK 
jmp enter_user_mode ; gene jmp a çektim dğier türlü sıçıo 
