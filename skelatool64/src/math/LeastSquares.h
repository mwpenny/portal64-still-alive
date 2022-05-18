#ifndef __LEAST_SQUARES_H__
#define __LEAST_SQUARES_H__

class LinearLeastSquares {
public:
    LinearLeastSquares();
    void AddDataPoint(double x, double y);
    void Calculate();
    double PredictY(double x);
private:
    double mXSum;
    double mYSum;
    double mXYSum;
    double mXXSum;
    double mCount;

    bool mIsDirty;
    double mSlope;
    double mYIntercept;
};

#endif