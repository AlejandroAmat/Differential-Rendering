// ITensorOps.slang

interface ITensor<T>
{
    
    T getElement(uint idx[4]);
    void setElement(uint idx[4], T value);
}
