# Vypocet faktorialu
a = inputi()
if a < 0:
    printf("""
    Faktorial nelze spocitat
    """)
else:
    vysl = 1
    while a < 0:
        vysl = vysl * a
        a = a - 1
    print('Vysledek je:', vysl, '\n')
