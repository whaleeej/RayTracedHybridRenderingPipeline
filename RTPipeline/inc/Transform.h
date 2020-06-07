#pragma once

#include <DX12LibPCH.h>

class Transform
{
public:
	struct MatCB
	{
		XMMATRIX ModelMatrix;
		XMMATRIX InverseTransposeModelMatrix;
		XMMATRIX ModelViewProjectionMatrix;
	};

public: 
	Transform(XMMATRIX trans, XMMATRIX rot, XMMATRIX scale) :
		translationMatrix(trans), rotationMatrix(rot), scaleMatrix(scale) {};
	XMMATRIX translationMatrix;
	XMMATRIX rotationMatrix;
	XMMATRIX scaleMatrix;

	XMMATRIX ComputeModel()
	{
		return scaleMatrix * rotationMatrix * translationMatrix;
	}
	MatCB ComputeMatCB(CXMMATRIX view, CXMMATRIX projection)
	{
		MatCB matCB;
		matCB.ModelMatrix = scaleMatrix * rotationMatrix * translationMatrix;
		matCB.InverseTransposeModelMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, matCB.ModelMatrix));
		matCB.ModelViewProjectionMatrix = matCB.ModelMatrix * view * projection;
		return matCB;
	}
};