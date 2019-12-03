# Vypocet faktorialu rekurzivne
def factorial(n):
    if n < 2:
        result = 1
    else:
        decremented_n = n - 1
        temp_result = factorial(decremented_n)
        result = n * temp_result
    return result

# Telo programu
#print('Zadajte cislo pre vypocet faktorialu')
#a = inputi()
#if a < 0.0:
#    print('Faktorial nelze spocitat')
#else:
#    vysl = factorial(a)
#    print('Vysledek je', vysl)
