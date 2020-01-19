#define f (1 << 14)

int inttofp(int);
int fptointzero(int);
int fptointnear(int);

int addfpfp(int,int);
int addfpint(int,int);
int subfpfp(int,int);
int subfpint(int,int);

int mulfpfp(int,int);
int mulfpint(int,int);
int divfpfp(int,int);
int divfpint(int,int);

int inttofp(int n)
{
int result=n*f;
return result;
}


int fptointzero(int x)
{
int result=x/f;
return result;
}

int fptointnear(int x)
{
int result=0;
if (x>=0)
	result=(x+f/2)/f;
if (x<=0)
	result=(x-f/2)/f;
return result;

}

int addfpfp(int x,int y)
{
int result=x+y;
return result;
}


int addfpint(int x,int n)
{

int result=x+(n*f);
return result;

}

int subfpfp(int x,int y)
{
int result=x-y;
return result;
}


int subfpint(int x,int n)
{

int result=x-(n*f);
return result;

}


int mulfpfp(int x,int y)
{
int result=((int64_t)x)*y/f;
return result;
}

int mulfpint(int x,int n)
{
int result=x*n;
return result;
}

int divfpfp(int x,int y)
{
int result=((int64_t)x)*f/y;
return result;
}

int divfpint(int x,int n)
{
int result=x/n;
return result;
}




