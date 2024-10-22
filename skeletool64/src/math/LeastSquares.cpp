#include "LeastSquares.h"

#include <math.h>

LinearLeastSquares::LinearLeastSquares() : 
    mXSum(0.0),
    mYSum(0.0),
    mXYSum(0.0),
    mXXSum(0.0),
    mCount(0.0),
    mIsDirty(false),
    mSlope(0.0),
    mYIntercept(0.0) {

}

void LinearLeastSquares::AddDataPoint(double x, double y) {
    mXSum += x;
    mYSum += y;
    mXYSum += x * y;
    mXXSum += x * x;
    mCount += 1.0;
    mIsDirty = true;
}

void LinearLeastSquares::Calculate() {
    if (!mIsDirty) {
        return;
    }

    mIsDirty = false;

    double denom = mCount * mXXSum - mXSum * mXSum;

    if (fabs(denom) < 0.00000001) {
        mSlope = 0.0f;
    } else {    
        mSlope = (mCount * mXYSum - mXSum * mYSum) / denom;
    }

    mYIntercept = (mYSum - mSlope * mXSum) / mCount;
}

double LinearLeastSquares::PredictY(double x) {
    Calculate();
    return mYIntercept + mSlope * x;
}