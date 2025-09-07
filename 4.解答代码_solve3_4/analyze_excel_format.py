# analyze_excel_format.py
# 详细分析现有的result1.xlsx和result2.xlsx文件格式和内容

import pandas as pd
import numpy as np
import os

def analyze_excel_file(filename):
    """详细分析Excel文件的格式和内容"""
    print(f"\n{'='*60}")
    print(f"分析文件: {filename}")
    print(f"{'='*60}")
    
    if not os.path.exists(filename):
        print(f"❌ 文件 {filename} 不存在")
        return
    
    try:
        # 读取Excel文件
        df = pd.read_excel(filename)
        
        # 基本信息
        print(f"📊 基本信息:")
        print(f"  - 行数: {len(df)}")
        print(f"  - 列数: {len(df.columns)}")
        print(f"  - 形状: {df.shape}")
        
        # 列名信息
        print(f"\n📝 列名列表:")
        for i, col in enumerate(df.columns):
            print(f"  [{i}] '{col}'")
        
        # 数据类型
        print(f"\n🔢 数据类型:")
        for col in df.columns:
            print(f"  {col}: {df[col].dtype}")
        
        # 完整数据内容
        print(f"\n📋 完整数据内容:")
        print("-" * 80)
        # 使用to_string()确保完整显示
        pd.set_option('display.max_columns', None)
        pd.set_option('display.max_rows', None)
        pd.set_option('display.width', None)
        pd.set_option('display.max_colwidth', None)
        print(df.to_string(index=True))
        
        # 非空数据统计
        print(f"\n📈 非空数据统计:")
        print(df.count())
        
        # 空值分布
        print(f"\n🕳️ 空值分布:")
        null_counts = df.isnull().sum()
        for col in df.columns:
            null_count = null_counts[col]
            if null_count > 0:
                print(f"  {col}: {null_count} 个空值")
        
        # 每行详细信息
        print(f"\n🔍 每行详细信息:")
        for idx, row in df.iterrows():
            print(f"  第{idx}行:")
            for col in df.columns:
                value = row[col]
                if pd.isna(value):
                    print(f"    {col}: [空值/NaN]")
                else:
                    print(f"    {col}: '{value}' (类型: {type(value).__name__})")
            print()
        
    except Exception as e:
        print(f"❌ 读取文件出错: {e}")

def main():
    print("🔍 Excel文件格式分析工具")
    print("分析现有的result1.xlsx和result2.xlsx文件格式")
    
    # 分析两个文件
    analyze_excel_file('result1.xlsx')
    analyze_excel_file('result2.xlsx')
    
    print(f"\n{'='*60}")
    print("✅ 分析完成！")
    print("请将上述输出结果完整复制给我，我将据此生成正确格式的文件。")
    print(f"{'='*60}")

if __name__ == '__main__':
    main()
