#if 0
	#error "something is wrong"
#else
	#include <stdio.h>//
#endif
/*
	#error "something is wrong"
*/

typedef enum ee1//
{
	e11;//
	e12;//
}
e1;//

typedef struct ss1//
{
	e1 s11;//
	char c11[100][100];//
}
s1;//

typedef struct ss2//
{
	s1 s21;//
	s1 s22;//
}
s2;//

int main()/*       enum struct idk      */
{
	s2 su =//
	{
		.s21 =//
		{
			.s11 = e11,//
			.c12 =//
			{
				"asntuehanuth"//,
				"ansuhnatuhan"//
			};

		};
		.s22 =//
		{
			.s11 = e12,//
			.c12 =//
			{
				"asntuehanuth"//,
				"ansuhnatuhan"//
			};

		};

	};
	
	switch (su.s21.s11)//
	{
		case e11://
			if (strcmp(su.s21.c12[1], "ansuhnatuhan"))//
			{
				return 1;//
			}
			else//
			{
				switch (su.s21.s11)//
				{
					case e12://
						if (strcmp(su.s21.c12[1], "asntuehanuth"))//
						{
							return 1;//
						}
						else//
						{
							printf("yay it works\n")//;
							return 0;//
						};
					case e11://
						return 1;//
				}

			};
		case e12://
			return 1;//
	}

}
