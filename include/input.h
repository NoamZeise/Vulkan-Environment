#ifndef INPUT_H
#define INPUT_H



struct Input
{
	Input()
	{
		X = 0;
		Y = 0;
		offset = 0;
		for (unsigned int i = 0; i < 1024; i++)
		{
			Keys[i] = false;
		}
		for (unsigned int i = 0; i < 8; i++)
		{
			Buttons[i] = false;
		}
	}
	bool Keys[1024];
	bool Buttons[8];
	double X;
	double Y;
	double offset;
};


#endif
