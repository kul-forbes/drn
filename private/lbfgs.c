/*
% Copyright (C) 2015-2016, Lorenzo Stella and Panagiotis Patrinos
%
% This file is part of ForBES.
%
% ForBES is free software: you can redistribute it and/or modify
% it under the terms of the GNU Lesser General Public License as published by
% the Free Software Foundation, either version 3 of the License, or
% (at your option) any later version.
%
% ForBES is distributed in the hope that it will be useful,
% but WITHOUT ANY WARRANTY; without even the implied warranty of
% MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
% GNU Lesser General Public License for more details.
%
% You should have received a copy of the GNU Lesser General Public License
% along with ForBES. If not, see <http://www.gnu.org/licenses/>.
*/

#include "mex.h"
#include "blas.h"

#define IS_REAL_SPARSE_MAT(P) (mxGetNumberOfDimensions(P) == 2 && \
    mxIsSparse(P) && mxIsDouble(P))
#define IS_REAL_DENSE_MAT(P) (mxGetNumberOfDimensions(P) == 2 && \
    !mxIsSparse(P) && mxIsDouble(P))
#define IS_REAL_DENSE_VEC(P) ((mxGetNumberOfDimensions(P) == 1 || \
    (mxGetNumberOfDimensions(P) == 2 && (mxGetN(P) == 1 || mxGetM(P) == 1))) && \
    !mxIsSparse(P) && mxIsDouble(P))
#define IS_INT32_DENSE_VEC(P) ((mxGetNumberOfDimensions(P) == 1 || \
    (mxGetNumberOfDimensions(P) == 2 && (mxGetN(P) == 1 || mxGetM(P) == 1))) && \
    !mxIsSparse(P) && mxIsInt32(P))
#define IS_REAL_SCALAR(P) (IS_REAL_DENSE_VEC(P) && mxGetNumberOfElements(P) == 1)
#define IS_INT32_SCALAR(P) (IS_INT32_DENSE_VEC(P) && mxGetNumberOfElements(P) == 1)
/* compile with mex -V lbfgs.c -lmwblas */
void LBFGS_MATVEC_TWOLOOP(int n, int mem, double * dir_n, double * s_n_m, double * y_n_m,
    double * ys_m, double H, double * g_n, int curridx, int currmem, double * alpha_m)
{
    double beta;
    int i, j, k;
    ptrdiff_t product_length = n; /* used with blas routines */
    ptrdiff_t one = 1; /* used with blas routines */

    dcopy(&product_length,g_n,&one,dir_n,&one);

    i = curridx;
    for (k=0; k<currmem; k++) {
		alpha_m[i] = ddot(&product_length, &(s_n_m[i*n]), &one, dir_n, &one)/ys_m[i];
        
        double minus_alpha = -alpha_m[i];
        daxpy(&product_length,&minus_alpha,&(y_n_m[i*n]),&one,dir_n,&one);
        i = i-1;
        if (i<0) i = mem-1;
    }

    dscal(&product_length,&H,dir_n,&one);

    i = i+1;
    if (i>=mem) i = 0;
    for (k=0; k<currmem; k++) {
        beta = ddot(&product_length,&(y_n_m[i*n]),&one,dir_n,&one)/ys_m[i];
        
        double alpha_minus_beta =alpha_m[i]-beta;
        daxpy(&product_length,&alpha_minus_beta,&(s_n_m[i*n]),&one,dir_n,&one);
        
        i = i+1;
        if (i>=mem) i = 0;
    }
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    int n, mem, curridx, currmem;
    double * dir, * s, * y, * ys, H, * g, * alpha;

    if (nrhs != 7) {
        mexErrMsgTxt("LBFGS: you should provide exactly 7 arguments.");
        return;
    }
    if (nlhs > 1) {
        mexErrMsgTxt("LBFGS: too many output arguments.");
        return;
    }
    if (!IS_REAL_DENSE_MAT(prhs[0])) {
        mexErrMsgTxt("LBFGS: 1st argument must be a double, dense matrix.");
        return;
    }
    if (!IS_REAL_DENSE_MAT(prhs[1])) {
        mexErrMsgTxt("LBFGS: 2nd argument must be a double, dense matrix.");
        return;
    }
    if (!IS_REAL_DENSE_VEC(prhs[2])) {
        mexErrMsgTxt("LBFGS: 3rd argument must be a double, dense vector.");
        return;
    }
    if (!IS_REAL_SCALAR(prhs[3])) {
        mexErrMsgTxt("LBFGS: 4th argument must be a double scalar.");
        return;
    }
    if (!IS_REAL_DENSE_VEC(prhs[4])) {
        mexErrMsgTxt("LBFGS: 5th argument must be a double, dense vector.");
        return;
    }
    if (!IS_INT32_SCALAR(prhs[5])) {
        mexErrMsgTxt("LBFGS: 6th argument must be a 32-bit integer.");
        return;
    }
    if (!IS_INT32_SCALAR(prhs[6])) {
        mexErrMsgTxt("LBFGS: 7th argument must be a 32-bit integer.");
        return;
    }

    s = mxGetPr(prhs[0]);
    y = mxGetPr(prhs[1]);
    ys = mxGetPr(prhs[2]);
    H = mxGetScalar(prhs[3]);
    g = mxGetPr(prhs[4]);
    curridx = (int)mxGetScalar(prhs[5])-1;
	currmem = (int)mxGetScalar(prhs[6]);

    n = mxGetDimensions(prhs[0])[0];
    const mwSize dir_dims[2] = { n ,1};
    mem = mxGetDimensions(prhs[0])[1];

	alpha = mxCalloc(mem, sizeof(double));

    plhs[0] = mxCreateNumericArray(2, dir_dims, mxDOUBLE_CLASS, mxREAL);
    dir = mxGetPr(plhs[0]);

    LBFGS_MATVEC_TWOLOOP(n, mem, dir, s, y, ys, H, g, curridx, currmem, alpha);

	mxFree(alpha);
}
