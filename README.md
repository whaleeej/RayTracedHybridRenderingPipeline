# Ray Traced Hybrid Rendering Pipeline  
## Demo Video Link  
-   [click me](http://47.100.57.110:8079/images/HybridRenderingPipelineDemo.mp4)
## Pipeline Design  
- Stages  
    - Raster Stage - G-Buffer Gen + Post Processing 
    - Ray Tracing Stage - Shaodw/AO/Indirect Illum Sampling
    - Compute Stage - Spatio Temporal Resample
- Pipeline Diagram  
![1_Pipeline_Design](./Assets/demo/1_Pipeline_Design.png)  
## Deferred Shading  
- G-Buffer Design  

    | RGB | A | RT |
    | :----:| :----: |:----:|
    |Position|	Hitness|	RT1|
    |Albedo|	Roughness|	RT2|
    |Normal| Metallic|	RT3|
    |Emissive|	Id|	RT4|
## Ray Tracing Sampeling  
- Acceleration Struture    
<img src="./Assets/demo/3_AS_Design.png" width="300" height="高度" alt="3_AS_Design">  
- Rays per sample pixel  
    - [1 shadow] + [1 ao] + [1 secondary] + [1 secondary shadow] spp  
- Shadow Sampling Illustration    
<img src="./Assets/demo/4_Shadow_Sample.png" width="300" height="高度" alt="4_Shadow_Sample">  
- Indirect Illumination Sampling Illustration    
<img src="./Assets/demo/5_Indirect_Sample.png" width="300" height="高度" alt="5_Indirect_Sample">  
## Computing Filtering and Reconstruction  
- Shadow&AO Reconstruction  
    - Pipeline Design    
    <img src="./Assets/demo/6_SVGF_design.png" width="300" height="高度" alt="6_SVGF_design">  
    - Illuntration    
    ![7_SVGF_filtered](./Assets/demo/7_SVGF_filtered.png)  
- Illumination Reconstruction  
    - Pipeline Design     
    <img src="./Assets/demo/8_BMFR_design.png" width="300" height="高度" alt="8_BMFR_design">  
    - Illuntration    
    ![9_BMFR_filtered](./Assets/demo/9_BMFR_filtered.png)  
## Build  
1. install visual studio 2019  
2. modify the "cd D:\Repos\DX12-HybridPipeline" in GenerateProjectFiles.bat to your root  
3. run the GenerateProjectFiles.bat script to build the vs project  