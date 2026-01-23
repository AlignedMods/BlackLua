float MY_FUNC(int var) {
    {
        return (float)var;
    }

    return 5.0f + (float)var;
}

int hello = -(int)(6.0f + (float)4.0);
float hi = (float)(hello + 6);

{
    int hi = 4, var = hi + 7;

    int funcVar = (int)MY_FUNC(8);
}