function [] = circleGen(x,y,r,colour)
%UNTITLED2 Summary of this function goes here
%   Detailed explanation goes here
points = 200;
n=zeros(200);
for i=1:points
    n(i)=2*pi/points*i;
end

fill(x+r.*cos(n), y+r.*sin(n),colour);
end

