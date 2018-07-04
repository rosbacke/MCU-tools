
BEGIN { print "uint32_t arg[] = {" }
/^#define  RCC_AHB1ENR/ {print "    " $2 }
/^#define  RCC_AHB2ENR/ {print "    " $2 }
/^#define  RCC_AHB3ENR/ {print "    " $2 }
/^#define  RCC_APB1ENR/ {print "    " $2 }
/^#define  RCC_APB2ENR/ {print "    " $2 }
END { print "}" }


/ RCC A/ { print "    " $3 }
