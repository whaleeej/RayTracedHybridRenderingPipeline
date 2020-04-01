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
![3_AS_Design](./Assets/demo/3_AS_Design.png)
- Rays per sample pixel
    - [1 shadow] + [1 ao] + [1 secondary] + [1 secondary shadow] spp
- Shadow Sampling Illustration
![4_Shadow_Sample](./Assets/demo/4_Shadow_Sample.png)
- Indirect Illumination Sampling Illustration
![5_Indirect_Sample](./Assets/demo/5_Indirect_Sample.png)
## Computing Filtering and Reconstruction
- Shadow&AO Reconstruction
    - Pipeline Design  
    ![6_SVGF_design](./Assets/demo/6_SVGF_design.png)
    - Illuntration  
    ![7_SVGF_filtered](./Assets/demo/7_SVGF_filtered.png)
- Illumination Reconstruction
    - Pipeline Design  
    ![8_BMFR_design](./Assets/demo/8_BMFR_design.png)
    - Illuntration  
    ![9_BMFR_filtered](./Assets/demo/9_BMFR_filtered.png)
## Build
1. install visual studio 2019
2. modify the "cd D:\Repos\DX12-HybridPipeline" in GenerateProjectFiles.bat to your root
3. run the GenerateProjectFiles.bat script to build the vs project